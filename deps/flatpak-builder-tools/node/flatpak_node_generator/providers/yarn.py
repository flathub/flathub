from pathlib import Path
from typing import Any, Dict, Iterator, List, Optional, Tuple, Type

import os
import re
import shlex
import types
import urllib.parse

from ..integrity import Integrity
from ..manifest import ManifestGenerator
from ..package import GitSource, LocalSource, Package, PackageSource, ResolvedSource
from . import LockfileProvider, ModuleProvider, ProviderFactory, RCFileProvider
from .npm import NpmRCFileProvider
from .special import SpecialSourceProvider

GIT_URL_PATTERNS = [
    re.compile(r'^git:'),
    re.compile(r'^git\+.+:'),
    re.compile(r'^ssh:'),
    re.compile(r'^https?:.+\.git$'),
    re.compile(r'^https?:.+\.git#.+'),
]

GIT_URL_HOSTS = ['github.com', 'gitlab.com', 'bitbucket.com', 'bitbucket.org']


class YarnLockfileProvider(LockfileProvider):
    _LOCAL_PKG_RE = re.compile(r'^(?:file|link):')

    @staticmethod
    def is_git_version(version: str) -> bool:
        for pattern in GIT_URL_PATTERNS:
            if pattern.match(version):
                return True
        url = urllib.parse.urlparse(version)
        if url.netloc in GIT_URL_HOSTS:
            return len([p for p in url.path.split('/') if p]) == 2
        return False

    def parse_lockfile(self, lockfile: Path) -> Dict[str, Any]:
        def _iter_lines() -> Iterator[Tuple[int, str]]:
            indent = '  '
            for line in lockfile.open():
                level = 0
                while line.startswith(indent):
                    level += 1
                    line = line[len(indent) :]
                yield level, line.strip()

        root_entry: Dict[str, Any] = {}
        parent_entries = [root_entry]

        for level, line in _iter_lines():
            if line.startswith('#') or not line:
                continue
            assert level <= len(parent_entries) - 1
            parent_entries = parent_entries[: level + 1]
            if line.endswith(':'):
                key = line[:-1]
                child_entry = parent_entries[-1][key] = {}
                parent_entries.append(child_entry)
            else:
                # NOTE shlex.split is handy, but slow;
                # to speed up parsing we can use something less robust, e.g.
                # _key, _value = line.split(' ', 1)
                # parent_entries[-1][self.unquote(_key)] = self.unquote(_value)
                key, value = shlex.split(line)
                parent_entries[-1][key] = value

        return root_entry

    def unquote(self, string: str) -> str:
        if string.startswith('"'):
            assert string.endswith('"')
            return string[1:-1]
        else:
            return string

    def process_package(
        self, lockfile: Path, name_line: str, entry: Dict[str, Any]
    ) -> Package:
        assert name_line and entry

        name = self.unquote(name_line.split(',', 1)[0])
        name, version_constraint = name.rsplit('@', 1)

        source: PackageSource
        if self._LOCAL_PKG_RE.match(version_constraint):
            source = LocalSource(path=self._LOCAL_PKG_RE.sub('', version_constraint))
        else:
            if self.is_git_version(entry['resolved']):
                source = self.parse_git_source(version=entry['resolved'])
            else:
                if 'integrity' in entry:
                    integrity = Integrity.parse(entry['integrity'])
                else:
                    integrity = None
                source = ResolvedSource(resolved=entry['resolved'], integrity=integrity)

        return Package(
            name=name, version=entry['version'], source=source, lockfile=lockfile
        )

    def process_lockfile(self, lockfile: Path) -> Iterator[Package]:
        for name_line, package in self.parse_lockfile(lockfile).items():
            yield self.process_package(lockfile, name_line, package)


class YarnRCFileProvider(RCFileProvider):
    RCFILE_NAME = '.yarnrc'


class YarnModuleProvider(ModuleProvider):
    # From https://github.com/yarnpkg/yarn/blob/v1.22.4/src/fetchers/tarball-fetcher.js
    _PACKAGE_TARBALL_URL_RE = re.compile(
        r'(?:(@[^/]+)(?:/|%2f))?[^/]+/(?:-|_attachments)/(?:@[^/]+/)?([^/]+)$'
    )

    def __init__(self, gen: ManifestGenerator, special: SpecialSourceProvider) -> None:
        self.gen = gen
        self.special_source_provider = special
        self.mirror_dir = self.gen.data_root / 'yarn-mirror'

    def __exit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_value: Optional[BaseException],
        tb: Optional[types.TracebackType],
    ) -> None:
        pass

    async def generate_package(self, package: Package) -> None:
        source = package.source

        if isinstance(source, ResolvedSource):
            integrity = await source.retrieve_integrity()
            url_parts = urllib.parse.urlparse(source.resolved)
            match = self._PACKAGE_TARBALL_URL_RE.search(url_parts.path)
            if match is not None:
                scope, filename = match.groups()
                if scope:
                    filename = f'{scope}-{filename}'
            else:
                filename = os.path.basename(url_parts.path)

            self.gen.add_url_source(
                source.resolved, integrity, self.mirror_dir / filename
            )

        elif isinstance(source, GitSource):
            repo_name = urllib.parse.urlparse(source.url).path.split('/')[-1]
            name = f'{repo_name}-{source.commit}'
            repo_dir = self.gen.tmp_root / name
            target_tar = os.path.relpath(self.mirror_dir / name, repo_dir)

            self.gen.add_git_source(source.url, source.commit, repo_dir)
            self.gen.add_command(f'mkdir -p {self.mirror_dir}')
            self.gen.add_command(
                f'cd {repo_dir}; git archive --format tar -o {target_tar} HEAD'
            )

        elif isinstance(source, LocalSource):
            assert (package.lockfile.parent / source.path / 'package.json').is_file()

        else:
            raise NotImplementedError(
                f'Unknown source type {source.__class__.__name__}'
            )

        await self.special_source_provider.generate_special_sources(package)


class YarnProviderFactory(ProviderFactory):
    def __init__(self) -> None:
        pass

    def create_lockfile_provider(self) -> YarnLockfileProvider:
        return YarnLockfileProvider()

    def create_rcfile_providers(self) -> List[RCFileProvider]:
        return [YarnRCFileProvider(), NpmRCFileProvider()]

    def create_module_provider(
        self, gen: ManifestGenerator, special: SpecialSourceProvider
    ) -> YarnModuleProvider:
        return YarnModuleProvider(gen, special)
