import json
import re
import sys
import types
from collections.abc import Iterator
from pathlib import Path
from typing import (
    Any,
    NamedTuple,
)

import yaml

from ..integrity import Integrity
from ..manifest import ManifestGenerator
from ..package import (
    GitSource,
    LocalSource,
    Lockfile,
    Package,
    PackageSource,
    ResolvedSource,
)
from . import LockfileProvider, ModuleProvider, ProviderFactory, RCFileProvider
from .npm import NpmRCFileProvider
from .special import SpecialSourceProvider

_V6_FORMAT_VERSIONS = {6, 7}
_SUPPORTED_VERSIONS = {6, 7, 9}

_STORE_VERSION_BY_LOCKFILE: dict[int, list[str]] = {
    6: ['v3'],
    7: ['v3'],
    9: ['v10', 'v11'],
}

_POPULATE_STORE_SCRIPT = Path(__file__).parents[1] / 'populate_pnpm_store.py'

STORE_VERSION_ARGUMENT_DEFAULT = _STORE_VERSION_BY_LOCKFILE[9][0]


class PnpmLockfileProvider(LockfileProvider):
    """Parses pnpm-lock.yaml (v6/v7 and v9) into Package objects."""

    _V6_PACKAGE_RE = re.compile(r'^/(?P<name>(?:@[^/]+/)?[^@]+)@(?P<version>[^(]+)')
    _V9_PACKAGE_RE = re.compile(r'^(?P<name>(?:@[^/]+/)?[^@]+)@(?P<version>[^(]+)')

    class Options(NamedTuple):
        no_devel: bool
        registry: str
        store_version: str | None = None

    def __init__(self, options: 'PnpmLockfileProvider.Options') -> None:
        self.no_devel = options.no_devel
        self.registry = options.registry.rstrip('/')
        self.store_version = options.store_version

    def _get_tarball_url(
        self,
        name: str,
        version: str,
        resolution: dict[str, Any],
    ) -> str:
        if 'tarball' in resolution:
            return str(resolution['tarball'])

        if name.startswith('@'):
            basename = name.split('/')[-1]
        else:
            basename = name

        return f'{self.registry}/{name}/-/{basename}-{version}.tgz'

    def _parse_package_key(self, key: str, major: int) -> tuple[str, str] | None:
        if major in _V6_FORMAT_VERSIONS:
            match = self._V6_PACKAGE_RE.match(key)
        else:
            match = self._V9_PACKAGE_RE.match(key)

        if match is None:
            return None
        return match.group('name'), match.group('version')

    def process_lockfile(self, lockfile_path: Path) -> Iterator[Package]:
        with open(lockfile_path, encoding='utf-8') as fp:
            data = yaml.safe_load(fp)

        raw_version = data.get('lockfileVersion')
        if raw_version is None:
            raise ValueError(f'{lockfile_path}: missing lockfileVersion field')

        major = int(str(raw_version).split('.', 1)[0])
        if major not in _SUPPORTED_VERSIONS:
            supported = ', '.join(str(v) for v in sorted(_SUPPORTED_VERSIONS))
            raise ValueError(
                f'{lockfile_path}: unsupported lockfileVersion {raw_version}. '
                f'Supported versions: {supported}.'
            )

        supported_store_versions = _STORE_VERSION_BY_LOCKFILE[major]
        store_version = self.store_version
        if store_version is None:
            store_version = supported_store_versions[0]
        elif store_version not in supported_store_versions:
            supported = ', '.join(str(v) for v in sorted(supported_store_versions))
            raise ValueError(
                f"{lockfile_path}: lockfileVersion {raw_version} doesn't support store version {self.store_version}. "
                f'Supported versions: {supported}.'
            )

        if self.no_devel and major not in _V6_FORMAT_VERSIONS:
            print(
                'WARNING: --no-devel is not yet supported for pnpm lockfile v9; '
                'all packages will be included.',
                file=sys.stderr,
            )

        lockfile = Lockfile(lockfile_path, major, store_version=store_version)

        packages_dict: dict[str, Any] = data.get('packages', {})
        if not packages_dict:
            return

        for key, info in packages_dict.items():
            if info is None:
                continue

            parsed = self._parse_package_key(key, major)
            if parsed is None:
                continue

            name, version = parsed

            # NOTE v6/v7: dev packages have dev: true directly on the entry
            if self.no_devel and info.get('dev', False):
                continue

            resolution: dict[str, Any] = info.get('resolution', {})
            if not resolution:
                continue

            source: PackageSource

            if resolution.get('type') == 'git':
                source = self.parse_git_source(
                    f'git+{resolution["repo"]}#{resolution["commit"]}'
                )
            elif 'directory' in resolution:
                source = LocalSource(path=resolution['directory'])
            else:
                integrity = None
                if 'integrity' in resolution:
                    integrity = Integrity.parse(resolution['integrity'])

                tarball_url = self._get_tarball_url(name, version, resolution)
                source = ResolvedSource(resolved=tarball_url, integrity=integrity)

            yield Package(
                name=name,
                version=version,
                source=source,
                lockfile=lockfile,
            )


class PnpmModuleProvider(ModuleProvider):
    """Generates flatpak sources for pnpm packages."""

    class _TarballInfo(NamedTuple):
        tarball_name: str
        name: str
        version: str
        integrity: Integrity

    def __init__(
        self,
        gen: ManifestGenerator,
        special: SpecialSourceProvider,
        lockfile_root: Path,
    ) -> None:
        self.gen = gen
        self.special_source_provider = special
        self.lockfile_root = lockfile_root
        self.tarball_dir = self.gen.data_root / 'pnpm-tarballs'
        self.store_dir = self.gen.data_root / 'pnpm-store'
        self._tarballs: list[PnpmModuleProvider._TarballInfo] = []
        self._store_version: str | None = None

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        tb: types.TracebackType | None,
    ) -> None:
        if exc_type is None:
            self._finalize()

    async def generate_package(self, package: Package) -> None:
        source = package.source

        sv = package.lockfile.store_version
        if sv is None:
            raise TypeError(
                f'{package.name}@{package.version}: lockfile provides no store version'
            )
        if self._store_version is None:
            self._store_version = sv
        elif self._store_version != sv:
            raise ValueError(
                f'{package.name}@{package.version}: store version mismatch: '
                f'expected {self._store_version!r}, got {sv!r}'
            )

        if isinstance(source, ResolvedSource):
            assert source.resolved is not None

            integrity = source.integrity
            if integrity is None:
                print(
                    f'INFO: {package.name}@{package.version}: '
                    'no integrity in lockfile, fetching to compute...',
                    file=sys.stderr,
                )
                integrity = await source.retrieve_integrity()

            tarball_name = f'{package.name.replace("/", "__")}-{package.version}.tgz'
            self.gen.add_url_source(
                url=source.resolved,
                integrity=integrity,
                destination=self.tarball_dir / tarball_name,
            )
            self._tarballs.append(
                self._TarballInfo(
                    tarball_name=tarball_name,
                    name=package.name,
                    version=package.version,
                    integrity=integrity,
                )
            )

            await self.special_source_provider.generate_special_sources(package)

        elif isinstance(source, GitSource):
            name = f'{package.name}-{source.commit}'
            path = self.gen.data_root / 'git-packages' / name
            self.gen.add_git_source(source.url, source.commit, path)

        elif isinstance(source, LocalSource):
            pass

        else:
            raise NotImplementedError(
                f'Unknown source type {source.__class__.__name__}'
            )

    def _finalize(self) -> None:
        if self._tarballs:
            self._add_store_population_script()
            self._add_pnpm_config()

    def _add_store_population_script(self) -> None:
        packages = {}
        for info in self._tarballs:
            entry: dict[str, str] = {
                'name': info.name,
                'version': info.version,
                'integrity': info.integrity.to_base64(),
                'integrity_digest': info.integrity.digest,
                'integrity_algo': info.integrity.algorithm,
            }
            if info.version.startswith(('http://', 'https://')):
                entry['tarball_url'] = info.version
            packages[info.tarball_name] = entry

        manifest = {
            'store_version': self._store_version,
            'packages': packages,
        }
        manifest_json = json.dumps(manifest, separators=(',', ':'), sort_keys=True)
        manifest_dest = self.gen.data_root / 'pnpm-manifest.json'
        self.gen.add_data_source(manifest_json, manifest_dest)

        with open(_POPULATE_STORE_SCRIPT, encoding='utf-8') as f:
            script_source = f.read()
        script_dest = self.gen.data_root / 'populate_pnpm_store.py'
        self.gen.add_data_source(script_source, script_dest)

        self.gen.add_command(
            f'python3 {script_dest} {manifest_dest} {self.tarball_dir} {self.store_dir}'
        )

    def _add_pnpm_config(self) -> None:
        if self._store_version == 'v11':
            self.gen.add_command(
                f'echo "storeDir=$PWD/{self.store_dir}" >> pnpm-workspace.yaml'
            )
        else:
            self.gen.add_command(f'echo "store-dir=$PWD/{self.store_dir}" >> .npmrc')


class PnpmProviderFactory(ProviderFactory):
    class Options(NamedTuple):
        lockfile: PnpmLockfileProvider.Options

    def __init__(
        self, lockfile_root: Path, options: 'PnpmProviderFactory.Options'
    ) -> None:
        self.lockfile_root = lockfile_root
        self.options = options

    def create_lockfile_provider(self) -> PnpmLockfileProvider:
        return PnpmLockfileProvider(self.options.lockfile)

    def create_rcfile_providers(self) -> list[RCFileProvider]:
        return [NpmRCFileProvider()]

    def create_module_provider(
        self, gen: ManifestGenerator, special: SpecialSourceProvider
    ) -> PnpmModuleProvider:
        return PnpmModuleProvider(gen, special, self.lockfile_root)
