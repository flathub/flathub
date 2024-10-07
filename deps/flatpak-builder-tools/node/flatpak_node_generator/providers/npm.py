from pathlib import Path
from typing import (
    Any,
    DefaultDict,
    Dict,
    Iterator,
    List,
    NamedTuple,
    Optional,
    Set,
    Type,
)

import asyncio
import collections
import functools
import json
import re
import shlex
import textwrap
import types
import urllib.parse

from ..integrity import Integrity
from ..manifest import ManifestGenerator
from ..package import (
    GitSource,
    LocalSource,
    Package,
    PackageSource,
    PackageURLSource,
    RegistrySource,
    ResolvedSource,
)
from ..requests import Requests
from ..url_metadata import RemoteUrlMetadata
from . import LockfileProvider, ModuleProvider, ProviderFactory, RCFileProvider
from .special import SpecialSourceProvider

_NPM_CORGIDOC = (
    'application/vnd.npm.install-v1+json; q=1.0, application/json; q=0.8, */*'
)


class NpmLockfileProvider(LockfileProvider):
    _ALIAS_RE = re.compile(r'^npm:(.[^@]*)@(.*)$')
    _PACKAGE_PREFIX_RE = re.compile(r'^(?P<prefix>[^@:]+@)[^@:]+:')

    class Options(NamedTuple):
        no_devel: bool

    def __init__(self, options: Options):
        self.no_devel = options.no_devel

    def _process_packages_v1(
        self, lockfile: Path, entry: Dict[str, Dict[Any, Any]]
    ) -> Iterator[Package]:
        for name, info in entry.get('dependencies', {}).items():
            if info.get('dev') and self.no_devel:
                continue
            elif info.get('bundled'):
                continue

            version: str = info['version']
            version_url = urllib.parse.urlparse(version)
            alias_match = self._ALIAS_RE.match(version)
            if alias_match is not None:
                name, version = alias_match.groups()

            source: PackageSource
            from_ = info.get('from')
            if from_ is not None:
                # Strip off the package name.
                match = self._PACKAGE_PREFIX_RE.match(from_)
                if match is not None:
                    from_ = from_[match.end('prefix') :]

                source = self.parse_git_source(version, from_)
            elif version_url.scheme == 'file':
                source = LocalSource(path=version_url.path)
            else:
                integrity = Integrity.parse(info['integrity'])
                if 'resolved' in info:
                    source = ResolvedSource(
                        resolved=info['resolved'], integrity=integrity
                    )
                elif version_url.scheme in {'http', 'https'}:
                    source = PackageURLSource(resolved=version, integrity=integrity)
                else:
                    source = RegistrySource(integrity=integrity)

            yield Package(name=name, version=version, source=source, lockfile=lockfile)

            if 'dependencies' in info:
                yield from self._process_packages_v1(lockfile, info)

    def _process_packages_v2(
        self, lockfile: Path, entry: Dict[str, Dict[Any, Any]]
    ) -> Iterator[Package]:
        for install_path, info in entry.get('packages', {}).items():
            if (info.get('dev') or info.get('devOptional')) and self.no_devel:
                continue
            if info.get('link'):
                # NOTE We're not interested in symlinks, NPM will create them at install time
                # but we still could collect package symlinks anyway just for completeness
                continue

            name = info.get('name')

            source: PackageSource
            package_json_path = lockfile.parent / install_path / 'package.json'
            if (
                'node_modules' not in package_json_path.parents
                and package_json_path.exists()
            ):
                source = LocalSource(path=install_path)
                if name is None:
                    with package_json_path.open('rb') as fp:
                        name = json.load(fp)['name']
            elif 'resolved' in info:
                resolved_url = urllib.parse.urlparse(info['resolved'])
                if resolved_url.scheme == 'file':
                    source = LocalSource(path=resolved_url.path)
                elif resolved_url.scheme in {'http', 'https'}:
                    integrity = Integrity.parse(info['integrity'])
                    # NOTE I don't know how to determine if the source came from a registry
                    # based on the lockfile alone, so unconditionally handling it as if it was
                    # a "URL as dependency" kind of source
                    source = PackageURLSource(
                        integrity=integrity, resolved=info['resolved']
                    )
                elif resolved_url.scheme.startswith('git+'):
                    raise NotImplementedError(
                        'Git sources in lockfile v2 format are not supported yet'
                        f' (package {install_path} in {lockfile})'
                    )
            else:
                raise NotImplementedError(
                    f"Don't know how to handle package {install_path} in {lockfile}"
                )

            # NOTE We can't reliably determine the package name from the lockfile v2 syntax,
            # but we need it for registry queries and special source processing;
            # If we didn't get the package name at this point, try determining it from
            # the install path as the last resort
            if name is None:
                path_list = install_path.split('/')
                name = '/'.join(path_list[-path_list[::-1].index('node_modules') :])

            yield Package(
                name=name,
                version=info.get('version'),
                lockfile=lockfile,
                source=source,
            )

    def process_lockfile(self, lockfile: Path) -> Iterator[Package]:
        with open(lockfile) as fp:
            data = json.load(fp)

        # TODO Once lockfile v2 syntax support is complete, use _process_packages_v2
        # for both v2 and v2 lockfiles
        if data['lockfileVersion'] in {1, 2}:
            yield from self._process_packages_v1(lockfile, data)
        elif data['lockfileVersion'] in {3}:
            yield from self._process_packages_v2(lockfile, data)
        else:
            raise NotImplementedError(
                f'Unknown lockfile version {data["lockfileVersion"]}'
            )


class NpmRCFileProvider(RCFileProvider):
    RCFILE_NAME = '.npmrc'


class NpmModuleProvider(ModuleProvider):
    class Options(NamedTuple):
        registry: str
        no_autopatch: bool
        no_trim_index: bool

    class RegistryPackageIndex(NamedTuple):
        url: str
        data: Dict[Any, Any]
        used_versions: Set[str]

    def __init__(
        self,
        gen: ManifestGenerator,
        special: SpecialSourceProvider,
        lockfile_root: Path,
        options: Options,
    ) -> None:
        self.gen = gen
        self.special_source_provider = special
        self.lockfile_root = lockfile_root
        self.registry = options.registry
        self.no_autopatch = options.no_autopatch
        self.no_trim_index = options.no_trim_index
        self.npm_cache_dir = self.gen.data_root / 'npm-cache'
        self.cacache_dir = self.npm_cache_dir / '_cacache'
        # Awaitable so multiple tasks can be waiting on the same package info.
        self.registry_packages: Dict[
            str, asyncio.Future[NpmModuleProvider.RegistryPackageIndex]
        ] = {}
        self.index_entries: Dict[Path, str] = {}
        self.all_lockfiles: Set[Path] = set()
        # Mapping of lockfiles to a dict of the Git source target paths and GitSource objects.
        self.git_sources: DefaultDict[
            Path, Dict[Path, GitSource]
        ] = collections.defaultdict(lambda: {})
        # FIXME better pass the same provider object we created in main
        self.rcfile_provider = NpmRCFileProvider()

    def __exit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_value: Optional[BaseException],
        tb: Optional[types.TracebackType],
    ) -> None:
        # Don't bother finalizing if an exception was thrown.
        if exc_type is None:
            self._finalize()

    def get_cacache_integrity_path(self, integrity: Integrity) -> Path:
        digest = integrity.digest
        return Path(digest[0:2]) / digest[2:4] / digest[4:]

    def get_cacache_index_path(self, integrity: Integrity) -> Path:
        return (
            self.cacache_dir
            / Path('index-v5')
            / self.get_cacache_integrity_path(integrity)
        )

    def get_cacache_content_path(self, integrity: Integrity) -> Path:
        return (
            self.cacache_dir
            / Path('content-v2')
            / integrity.algorithm
            / self.get_cacache_integrity_path(integrity)
        )

    def add_index_entry(
        self,
        url: str,
        metadata: RemoteUrlMetadata,
        request_headers: Dict[str, str] = {},
    ) -> None:
        key = f'make-fetch-happen:request-cache:{url}'

        index_json = json.dumps(
            {
                'key': key,
                'integrity': f'{metadata.integrity.algorithm}-{metadata.integrity.to_base64()}',
                'time': 0,
                'size': metadata.size,
                'metadata': {
                    'url': url,
                    'reqHeaders': request_headers,
                    'resHeaders': {},
                },
            }
        )

        content_integrity = Integrity.generate(index_json, algorithm='sha1')
        index = '\t'.join((content_integrity.digest, index_json))

        key_integrity = Integrity.generate(key)
        index_path = self.get_cacache_index_path(key_integrity)
        self.index_entries[index_path] = index

    async def resolve_source(self, package: Package) -> ResolvedSource:
        assert isinstance(package.source, RegistrySource)

        # These results are going to be the same each time.
        if package.name not in self.registry_packages:
            cache_future = asyncio.get_event_loop().create_future()
            self.registry_packages[package.name] = cache_future

            data_url = f'{self.get_package_registry(package)}/{package.name.replace("/", "%2f")}'
            # NOTE: Not cachable, because this is an API call.
            raw_data = await Requests.instance.read_all(data_url, cachable=False)
            data = json.loads(raw_data)

            assert 'versions' in data, f'{data_url} returned an invalid package index'
            cache_future.set_result(
                NpmModuleProvider.RegistryPackageIndex(
                    url=data_url, data=data, used_versions=set()
                )
            )

            if not self.no_trim_index:
                for key in list(data):
                    if key != 'versions':
                        del data[key]

        index = await self.registry_packages[package.name]

        versions = index.data['versions']
        assert (
            package.version in versions
        ), f'{package.name} versions available are {", ".join(versions)}, not {package.version}'

        dist = versions[package.version]['dist']
        assert (
            'tarball' in dist
        ), f'{package.name}@{package.version} has no tarball in dist'

        index.used_versions.add(package.version)

        registry_integrity: Integrity
        if 'integrity' in dist:
            registry_integrity = Integrity.parse(dist['integrity'])
        elif 'shasum' in dist:
            registry_integrity = Integrity.from_sha1(dist['shasum'])
        else:
            assert False, f'{package.name}@{package.version} has no integrity in dist'

        if package.source.integrity:
            # Follow npm in only checking for a matching integrity if the algorithms are
            # the same:
            # https://github.com/npm/pacote/blob/e48370d441b8d8eef3080e5d47c8ab6a8cc2aca0/lib/registry.js#L143
            if (
                package.source.integrity.algorithm == registry_integrity.algorithm
                and package.source.integrity.digest != registry_integrity.digest
            ):
                raise ValueError(
                    f"{package.name}@{package.version} integrity doesn't match registry integrity"
                )

            integrity = package.source.integrity
        else:
            integrity = registry_integrity

        return ResolvedSource(resolved=dist['tarball'], integrity=integrity)

    async def generate_package(self, package: Package) -> None:
        self.all_lockfiles.add(package.lockfile)
        source = package.source

        if isinstance(source, (RegistrySource, PackageURLSource)):
            if isinstance(source, RegistrySource):
                source = await self.resolve_source(package)

            assert source.resolved is not None
            assert source.integrity is not None

            self.gen.add_url_source(
                url=source.resolved,
                integrity=source.integrity,
                destination=self.get_cacache_content_path(source.integrity),
            )
            self.add_index_entry(
                url=source.resolved,
                metadata=RemoteUrlMetadata(
                    integrity=source.integrity,
                    size=await RemoteUrlMetadata.get_size(
                        source.resolved, cachable=True
                    ),
                ),
            )

            await self.special_source_provider.generate_special_sources(package)

        # pyright: reportUnnecessaryIsInstance=false
        elif isinstance(source, GitSource):
            # Get a unique name to use for the Git repository folder.
            name = f'{package.name}-{source.commit}'
            path = self.gen.data_root / 'git-packages' / name
            self.git_sources[package.lockfile][path] = source
            self.gen.add_git_source(source.url, source.commit, path)

        elif isinstance(source, LocalSource):
            pass

        else:
            raise NotImplementedError(
                f'Unknown source type {source.__class__.__name__}'
            )

    def relative_lockfile_dir(self, lockfile: Path) -> Path:
        return lockfile.parent.relative_to(self.lockfile_root)

    @functools.lru_cache(typed=True)
    def get_lockfile_rc(self, lockfile: Path) -> Dict[str, str]:
        rc = {}
        rcfile_path = lockfile.parent / self.rcfile_provider.RCFILE_NAME
        if rcfile_path.is_file():
            rc.update(self.rcfile_provider.parse_rcfile(rcfile_path))
        return rc

    def get_package_registry(self, package: Package) -> str:
        assert isinstance(package.source, RegistrySource)
        rc = self.get_lockfile_rc(package.lockfile)
        if rc and '/' in package.name:
            scope, _ = package.name.split('/', maxsplit=1)
            if f'{scope}:registry' in rc:
                return rc[f'{scope}:registry']
        return self.registry

    def _finalize(self) -> None:
        for _, async_index in self.registry_packages.items():
            index = async_index.result()

            if not self.no_trim_index:
                for version in list(index.data['versions'].keys()):
                    if version not in index.used_versions:
                        del index.data['versions'][version]

            raw_data = json.dumps(index.data).encode()

            metadata = RemoteUrlMetadata(
                integrity=Integrity.generate(raw_data), size=len(raw_data)
            )
            content_path = self.get_cacache_content_path(metadata.integrity)
            self.gen.add_data_source(raw_data, content_path)
            self.add_index_entry(
                index.url, metadata, request_headers={'accept': _NPM_CORGIDOC}
            )

        patch_commands: DefaultDict[Path, List[str]] = collections.defaultdict(
            lambda: []
        )

        if self.git_sources:
            # Generate jq scripts to patch the package*.json files.
            scripts = {
                'package.json': r"""
                    walk(
                        if type == "object"
                        then
                            to_entries | map(
                                if (.value | type == "string") and $data[.value]
                                then .value = "git+file:\($buildroot)/\($data[.value])"
                                else .
                                end
                            ) | from_entries
                        else .
                        end
                    )
                """,
                'package-lock.json': r"""
                    walk(
                        if type == "object" and (.version | type == "string") and $data[.version]
                        then
                            .version = "git+file:\($buildroot)/\($data[.version])"
                        else .
                        end
                    )
                """,
            }

            for lockfile, sources in self.git_sources.items():
                prefix = self.relative_lockfile_dir(lockfile)
                data: Dict[str, Dict[str, str]] = {
                    'package.json': {},
                    'package-lock.json': {},
                }

                for path, source in sources.items():
                    GIT_URL_PREFIX = 'git+'

                    new_version = f'{path}#{source.commit}'
                    assert source.from_ is not None
                    data['package.json'][source.from_] = new_version
                    data['package-lock.json'][source.original] = new_version

                    if source.from_.startswith(GIT_URL_PREFIX):
                        data['package.json'][
                            source.from_[len(GIT_URL_PREFIX) :]
                        ] = new_version

                    if source.original.startswith(GIT_URL_PREFIX):
                        data['package-lock.json'][
                            source.original[len(GIT_URL_PREFIX) :]
                        ] = new_version

                for filename, script in scripts.items():
                    target = Path('$FLATPAK_BUILDER_BUILDDIR') / prefix / filename
                    script = (
                        textwrap.dedent(script.lstrip('\n')).strip().replace('\n', '')
                    )
                    json_data = json.dumps(data[filename])
                    patch_commands[lockfile].append(
                        'jq'
                        ' --arg buildroot "$FLATPAK_BUILDER_BUILDDIR"'
                        f' --argjson data {shlex.quote(json_data)}'
                        f' {shlex.quote(script)} {target}'
                        f' > {target}.new'
                    )
                    patch_commands[lockfile].append(f'mv {target}{{.new,}}')

        patch_all_commands: List[str] = []
        for lockfile in self.all_lockfiles:
            patch_dest = (
                self.gen.data_root / 'patch' / self.relative_lockfile_dir(lockfile)
            )
            # Don't use with_extension to avoid problems if the package has a . in its name.
            patch_dest = patch_dest.with_name(patch_dest.name + '.sh')

            self.gen.add_script_source(patch_commands[lockfile], patch_dest)
            patch_all_commands.append(f'$FLATPAK_BUILDER_BUILDDIR/{patch_dest}')

        patch_all_dest = self.gen.data_root / 'patch-all.sh'
        self.gen.add_script_source(patch_all_commands, patch_all_dest)

        if not self.no_autopatch:
            # FLATPAK_BUILDER_BUILDDIR isn't defined yet for script sources.
            self.gen.add_command(f'FLATPAK_BUILDER_BUILDDIR=$PWD {patch_all_dest}')

        if self.index_entries:
            for path, entry in self.index_entries.items():
                self.gen.add_data_source(entry, path)


class NpmProviderFactory(ProviderFactory):
    class Options(NamedTuple):
        lockfile: NpmLockfileProvider.Options
        module: NpmModuleProvider.Options

    def __init__(self, lockfile_root: Path, options: Options) -> None:
        self.lockfile_root = lockfile_root
        self.options = options

    def create_lockfile_provider(self) -> NpmLockfileProvider:
        return NpmLockfileProvider(self.options.lockfile)

    def create_rcfile_providers(self) -> List[RCFileProvider]:
        return [NpmRCFileProvider()]

    def create_module_provider(
        self, gen: ManifestGenerator, special: SpecialSourceProvider
    ) -> NpmModuleProvider:
        return NpmModuleProvider(gen, special, self.lockfile_root, self.options.module)
