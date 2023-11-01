#!/usr/bin/env python3

__license__ = 'MIT'
import json
from urllib.parse import urlparse, ParseResult, parse_qs
import os
import contextlib
import copy
import glob
import subprocess
import argparse
import logging
import hashlib
import asyncio
from typing import Any, Dict, List, NamedTuple, Optional, Tuple, TypedDict

import aiohttp
import toml

CRATES_IO = 'https://static.crates.io/crates'
CARGO_HOME = 'cargo'
CARGO_CRATES = f'{CARGO_HOME}/vendor'
VENDORED_SOURCES = 'vendored-sources'
GIT_CACHE = 'flatpak-cargo/git'
COMMIT_LEN = 7


@contextlib.contextmanager
def workdir(path: str):
    oldpath = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(oldpath)


def canonical_url(url: str) -> ParseResult:
    'Converts a string to a Cargo Canonical URL, as per https://github.com/rust-lang/cargo/blob/35c55a93200c84a4de4627f1770f76a8ad268a39/src/cargo/util/canonical_url.rs#L19'
    # Hrm. The upstream cargo does not replace those URLs, but if we don't then it doesn't work too well :(
    url = url.replace('git+https://', 'https://')
    u = urlparse(url)
    # It seems cargo drops query and fragment
    u = ParseResult(u.scheme, u.netloc, u.path, '', '', '')
    u = u._replace(path = u.path.rstrip('/'))

    if u.netloc == 'github.com':
        u = u._replace(scheme = 'https')
        u = u._replace(path = u.path.lower())

    if u.path.endswith('.git'):
        u = u._replace(path = u.path[:-len('.git')])

    return u


def get_git_tarball(repo_url: str, commit: str) -> str:
    url = canonical_url(repo_url)
    path = url.path.split('/')[1:]

    assert len(path) == 2
    owner = path[0]
    if path[1].endswith('.git'):
        repo = path[1].replace('.git', '')
    else:
        repo = path[1]
    if url.hostname == 'github.com':
        return f'https://codeload.{url.hostname}/{owner}/{repo}/tar.gz/{commit}'
    elif url.hostname.split('.')[0] == 'gitlab':  # type: ignore
        return f'https://{url.hostname}/{owner}/{repo}/-/archive/{commit}/{repo}-{commit}.tar.gz'
    elif url.hostname == 'bitbucket.org':
        return f'https://{url.hostname}/{owner}/{repo}/get/{commit}.tar.gz'
    else:
        raise ValueError(f'Don\'t know how to get tarball for {repo_url}')


async def get_remote_sha256(url: str) -> str:
    logging.info(f"started sha256({url})")
    sha256 = hashlib.sha256()
    async with aiohttp.ClientSession(raise_for_status=True) as http_session:
        async with http_session.get(url) as response:
            while True:
                data = await response.content.read(4096)
                if not data:
                    break
                sha256.update(data)
    logging.info(f"done sha256({url})")
    return sha256.hexdigest()


_TomlType = Dict[str, Any]


def load_toml(tomlfile: str = 'Cargo.lock') -> _TomlType:
    with open(tomlfile, 'r') as f:
        toml_data = toml.load(f)
    return toml_data


def git_repo_name(git_url: str, commit: str) -> str:
    name = canonical_url(git_url).path.split('/')[-1]
    return f'{name}-{commit[:COMMIT_LEN]}'


def fetch_git_repo(git_url: str, commit: str) -> str:
    repo_dir = git_url.replace('://', '_').replace('/', '_')
    cache_dir = os.environ.get('XDG_CACHE_HOME', os.path.expanduser('~/.cache'))
    clone_dir = os.path.join(cache_dir, 'flatpak-cargo', repo_dir)
    if not os.path.isdir(os.path.join(clone_dir, '.git')):
        subprocess.run(['git', 'clone', '--depth=1', git_url, clone_dir], check=True)
    rev_parse_proc = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=clone_dir, check=True,
                                    stdout=subprocess.PIPE)
    head = rev_parse_proc.stdout.decode().strip()
    if head[:COMMIT_LEN] != commit[:COMMIT_LEN]:
        subprocess.run(['git', 'fetch', 'origin', commit], cwd=clone_dir, check=True)
        subprocess.run(['git', 'checkout', commit], cwd=clone_dir, check=True)
    return clone_dir


class _GitPackage(NamedTuple):
    path: str
    package: _TomlType
    workspace: Optional[_TomlType]

    @property
    def normalized(self) -> _TomlType:
        package = copy.deepcopy(self.package)
        if self.workspace is None:
            return package
        for section_key, section in package.items():
            # XXX We ignore top-level lists here; maybe we should iterate over list items, too
            if not isinstance(section, dict):
                continue
            for key, value in section.items():
                if not isinstance(value, dict):
                    continue
                if not value.get('workspace'):
                    continue
                package[section_key][key] = self.workspace[section_key][key]
        return package


_GitPackagesType = Dict[str, _GitPackage]


async def get_git_repo_packages(git_url: str, commit: str) -> _GitPackagesType:
    logging.info('Loading packages from %s', git_url)
    git_repo_dir = fetch_git_repo(git_url, commit)
    packages: _GitPackagesType = {}

    with workdir(git_repo_dir):
        if os.path.isfile('Cargo.toml'):
            packages.update(await get_cargo_toml_packages(load_toml('Cargo.toml'), '.'))
        else:
            for toml_path in glob.glob('*/Cargo.toml'):
                packages.update(await get_cargo_toml_packages(load_toml(toml_path),
                                                              os.path.dirname(toml_path)))

    assert packages, f"No packages found in {git_repo_dir}"
    logging.debug(
        'Packages in %s:\n%s',
        git_url,
        json.dumps(
            {k: v.path for k, v in packages.items()},
            indent=4,
        ),
    )
    return packages


async def get_cargo_toml_packages(root_toml: _TomlType, root_dir: str) -> _GitPackagesType:
    assert not os.path.isabs(root_dir) and os.path.isdir(root_dir)
    assert 'package' in root_toml or 'workspace' in root_toml
    packages: _GitPackagesType = {}

    async def get_dep_packages(
        entry: _TomlType,
        toml_dir: str,
        workspace: Optional[_TomlType] = None,
    ):
        assert not os.path.isabs(toml_dir)
        # https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html
        if 'dependencies' in entry:
            for dep_name, dep in entry['dependencies'].items():
                if 'package' in dep:
                    dep_name = dep['package']
                if 'path' not in dep:
                    continue
                if dep_name in packages:
                    continue
                dep_dir = os.path.normpath(os.path.join(toml_dir, dep['path']))
                logging.debug("Loading dependency %s from %s", dep_name, dep_dir)
                dep_toml = load_toml(os.path.join(dep_dir, 'Cargo.toml'))
                assert dep_toml['package']['name'] == dep_name, toml_dir
                await get_dep_packages(dep_toml, dep_dir, workspace)
                packages[dep_name] = _GitPackage(
                    path=dep_dir,
                    package=dep,
                    workspace=workspace,
                )
        if 'target' in entry:
            for _, target in entry['target'].items():
                await get_dep_packages(target, toml_dir)

    if 'package' in root_toml:
        await get_dep_packages(root_toml, root_dir)
        packages[root_toml['package']['name']] = _GitPackage(
            path=root_dir,
            package=root_toml,
            workspace=None,
        )

    if 'workspace' in root_toml:
        for member in root_toml['workspace'].get('members', []):
            for subpkg_toml in glob.glob(os.path.join(root_dir, member, 'Cargo.toml')):
                subpkg = os.path.normpath(os.path.dirname(subpkg_toml))
                logging.debug(
                    "Loading workspace member %s in %s",
                    subpkg_toml,
                    os.path.abspath(root_dir),
                )
                pkg_toml = load_toml(subpkg_toml)
                await get_dep_packages(pkg_toml, subpkg, root_toml['workspace'])
                packages[pkg_toml['package']['name']] = _GitPackage(
                    path=subpkg,
                    package=pkg_toml,
                    workspace=root_toml['workspace'],
                )

    return packages


_FlatpakSourceType = Dict[str, Any]


async def get_git_repo_sources(
    url: str,
    commit: str,
    tarball: bool = False,
) -> List[_FlatpakSourceType]:
    name = git_repo_name(url, commit)
    if tarball:
        tarball_url = get_git_tarball(url, commit)
        git_repo_sources = [{
            'type': 'archive',
            'archive-type': 'tar-gzip',
            'url': tarball_url,
            'sha256': await get_remote_sha256(tarball_url),
            'dest': f'{GIT_CACHE}/{name}',
        }]
    else:
        git_repo_sources = [{
            'type': 'git',
            'url': url,
            'commit': commit,
            'dest': f'{GIT_CACHE}/{name}',
        }]
    return git_repo_sources


_GitRepo = TypedDict('_GitRepo', {'lock': asyncio.Lock, 'commits': Dict[str, _GitPackagesType]})
_GitReposType = Dict[str, _GitRepo]
_VendorEntryType = Dict[str, Dict[str, str]]


async def get_git_package_sources(
    package: _TomlType,
    git_repos: _GitReposType,
) -> Tuple[List[_FlatpakSourceType], _VendorEntryType]:
    name = package['name']
    source = package['source']
    commit = urlparse(source).fragment
    assert commit, 'The commit needs to be indicated in the fragement part'
    canonical = canonical_url(source)
    repo_url = canonical.geturl()

    git_repo = git_repos.setdefault(repo_url, {
        'commits': {},
        'lock': asyncio.Lock(),
    })
    async with git_repo['lock']:
        if commit not in git_repo['commits']:
            git_repo['commits'][commit] = await get_git_repo_packages(repo_url, commit)

    cargo_vendored_entry: _VendorEntryType = {
        repo_url: {
            'git': repo_url,
            'replace-with': VENDORED_SOURCES,
        }
    }
    rev = parse_qs(urlparse(source).query).get('rev')
    tag = parse_qs(urlparse(source).query).get('tag')
    branch = parse_qs(urlparse(source).query).get('branch')
    if rev:
        assert len(rev) == 1
        cargo_vendored_entry[repo_url]['rev'] = rev[0]
    elif tag:
        assert len(tag) == 1
        cargo_vendored_entry[repo_url]['tag'] = tag[0]
    elif branch:
        assert len(branch) == 1
        cargo_vendored_entry[repo_url]['branch'] = branch[0]

    logging.info("Adding package %s from %s", name, repo_url)
    git_pkg = git_repo['commits'][commit][name]
    pkg_repo_dir = os.path.join(GIT_CACHE, git_repo_name(repo_url, commit), git_pkg.path)
    git_sources: List[_FlatpakSourceType] = [
        {
            'type': 'shell',
            'commands': [
                f'cp -r --reflink=auto "{pkg_repo_dir}" "{CARGO_CRATES}/{name}"'
            ],
        },
        {
            'type': 'inline',
            'contents': toml.dumps(git_pkg.normalized),
            'dest': f'{CARGO_CRATES}/{name}', #-{version}',
            'dest-filename': 'Cargo.toml',
        },
        {
            'type': 'inline',
            'contents': json.dumps({'package': None, 'files': {}}),
            'dest': f'{CARGO_CRATES}/{name}', #-{version}',
            'dest-filename': '.cargo-checksum.json',
        }
    ]

    return (git_sources, cargo_vendored_entry)


async def get_package_sources(
    package: _TomlType,
    cargo_lock: _TomlType,
    git_repos: _GitReposType,
) -> Optional[Tuple[List[_FlatpakSourceType], _VendorEntryType]]:
    metadata = cargo_lock.get('metadata')
    name = package['name']
    version = package['version']

    if 'source' not in package:
        logging.debug('%s has no source', name)
        return None
    source = package['source']

    if source.startswith('git+'):
        return await get_git_package_sources(package, git_repos)

    key = f'checksum {name} {version} ({source})'
    if metadata is not None and key in metadata:
        checksum = metadata[key]
    elif 'checksum' in package:
        checksum = package['checksum']
    else:
        logging.warning(f'{name} doesn\'t have checksum')
        return None
    crate_sources = [
        {
            'type': 'archive',
            'archive-type': 'tar-gzip',
            'url': f'{CRATES_IO}/{name}/{name}-{version}.crate',
            'sha256': checksum,
            'dest': f'{CARGO_CRATES}/{name}-{version}',
        },
        {
            'type': 'inline',
            'contents': json.dumps({'package': checksum, 'files': {}}),
            'dest': f'{CARGO_CRATES}/{name}-{version}',
            'dest-filename': '.cargo-checksum.json',
        },
    ]
    return (crate_sources, {'crates-io': {'replace-with': VENDORED_SOURCES}})


async def generate_sources(
    cargo_lock: _TomlType,
    git_tarballs: bool = False,
) -> List[_FlatpakSourceType]:
    # {
    #     "git-repo-url": {
    #         "lock": asyncio.Lock(),
    #         "commits": {
    #             "commit-hash": {
    #                 "package-name": "./relative/package/path"
    #             }
    #         }
    #     }
    # }
    git_repos: _GitReposType = {}
    sources: List[_FlatpakSourceType] = []
    package_sources = []
    cargo_vendored_sources = {
        VENDORED_SOURCES: {'directory': f'{CARGO_CRATES}'},
    }

    pkg_coros = [get_package_sources(p, cargo_lock, git_repos) for p in cargo_lock['package']]
    for pkg in await asyncio.gather(*pkg_coros):
        if pkg is None:
            continue
        else:
            pkg_sources, cargo_vendored_entry = pkg
        package_sources.extend(pkg_sources)
        cargo_vendored_sources.update(cargo_vendored_entry)

    logging.debug('Adding collected git repos:\n%s', json.dumps(list(git_repos), indent=4))
    git_repo_coros = []
    for git_url, git_repo in git_repos.items():
        for git_commit in git_repo['commits']:
            git_repo_coros.append(get_git_repo_sources(git_url, git_commit, git_tarballs))
    sources.extend(sum(await asyncio.gather(*git_repo_coros), []))

    sources.extend(package_sources)

    logging.debug('Vendored sources:\n%s', json.dumps(cargo_vendored_sources, indent=4))
    sources.append({
        'type': 'inline',
        'contents': toml.dumps({
            'source': cargo_vendored_sources,
        }),
        'dest': CARGO_HOME,
        'dest-filename': 'config'
    })
    return sources


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('cargo_lock', help='Path to the Cargo.lock file')
    parser.add_argument('-o', '--output', required=False, help='Where to write generated sources')
    parser.add_argument('-t', '--git-tarballs', action='store_true', help='Download git repos as tarballs')
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()
    if args.output is not None:
        outfile = args.output
    else:
        outfile = 'generated-sources.json'
    if args.debug:
        loglevel = logging.DEBUG
    else:
        loglevel = logging.INFO
    logging.basicConfig(level=loglevel)

    generated_sources = asyncio.run(generate_sources(load_toml(args.cargo_lock),
                                    git_tarballs=args.git_tarballs))
    with open(outfile, 'w') as out:
        json.dump(generated_sources, out, indent=4, sort_keys=False)


if __name__ == '__main__':
    main()
