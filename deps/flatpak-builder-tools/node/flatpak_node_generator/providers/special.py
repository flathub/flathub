from pathlib import Path
from typing import DefaultDict, List, NamedTuple, Optional, Tuple

import collections
import hashlib
import itertools
import json
import os
import re
import urllib.parse

from ..electron import ElectronBinaryManager
from ..integrity import Integrity
from ..manifest import ManifestGenerator
from ..node_headers import NodeHeaders
from ..package import Package, SemVer
from ..requests import Requests, StubRequests
from ..url_metadata import RemoteUrlMetadata

_NPM_MIRROR = 'https://unpkg.com/'


class SpecialSourceProvider:
    class Options(NamedTuple):
        node_chromedriver_from_electron: Optional[str]
        electron_ffmpeg: Optional[str]
        electron_node_headers: bool
        nwjs_version: Optional[str]
        nwjs_node_headers: bool
        nwjs_ffmpeg: bool
        xdg_layout: bool

    def __init__(self, gen: ManifestGenerator, options: Options):
        self.gen = gen
        self.node_chromedriver_from_electron = options.node_chromedriver_from_electron
        self.electron_ffmpeg = options.electron_ffmpeg
        self.electron_node_headers = options.electron_node_headers
        self.nwjs_version = options.nwjs_version
        self.nwjs_node_headers = options.nwjs_node_headers
        self.nwjs_ffmpeg = options.nwjs_ffmpeg
        self.xdg_layout = options.xdg_layout

    @property
    def electron_cache_dir(self) -> Path:
        if self.xdg_layout:
            return self.gen.data_root / 'cache' / 'electron'
        return self.gen.data_root / 'electron-cache'

    @property
    def gyp_dir(self) -> Path:
        return self.gen.data_root / 'cache' / 'node-gyp'

    def _add_electron_cache_downloads(
        self,
        manager: ElectronBinaryManager,
        binary_name: str,
        *,
        add_integrities: bool = True,
    ) -> None:
        electron_cache_dir = self.electron_cache_dir
        links_to_create: DefaultDict[
            str, List[Tuple[ElectronBinaryManager.Binary, str]]
        ] = collections.defaultdict(lambda: [])

        for binary in manager.find_binaries(binary_name):
            assert binary.arch is not None
            self.gen.add_url_source(
                binary.url,
                binary.integrity,
                electron_cache_dir / binary.filename,
                only_arches=[binary.arch.flatpak],
            )
            # Symlinks for @electron/get, which stores electron zips in a subdir
            if self.xdg_layout:
                sanitized_url = ''.join(c for c in binary.url if c not in '/:')

                links_to_create[sanitized_url].append((binary, binary.filename))
                # And for @electron/get >= 1.12.4 its sha256 hash of url dirname
                links_to_create[binary.url_hash].append((binary, binary.filename))

        if add_integrities:
            integrity_file = manager.integrity_file
            self.gen.add_url_source(
                integrity_file.url,
                integrity_file.integrity,
                electron_cache_dir / integrity_file.filename,
            )
            links_to_create[integrity_file.url_hash].append(
                (integrity_file, ElectronBinaryManager.INTEGRITY_BASE_FILENAME)
            )

        for dir, all_binaries in links_to_create.items():
            for arch, binaries_it in itertools.groupby(
                sorted(all_binaries, key=lambda b: str(b[0].arch)),
                key=lambda b: b[0].arch,
            ):
                binaries = list(binaries_it)
                self.gen.add_shell_source(
                    [
                        f'mkdir -p "{dir}"',
                        *(
                            f'ln -s "../{binary.filename}" "{dir}/{dest}"'
                            for (binary, dest) in binaries
                        ),
                    ],
                    destination=electron_cache_dir,
                    only_arches=[arch.flatpak] if arch is not None else None,
                )

    async def _handle_electron(self, package: Package) -> None:
        manager = await ElectronBinaryManager.for_version(package.version)
        self._add_electron_cache_downloads(manager, 'electron')

        if self.electron_ffmpeg is not None:
            if self.electron_ffmpeg == 'archive':
                self._add_electron_cache_downloads(
                    manager, 'ffmpeg', add_integrities=False
                )
            elif self.electron_ffmpeg == 'lib':
                for binary in manager.find_binaries('ffmpeg'):
                    assert binary.arch is not None
                    self.gen.add_archive_source(
                        binary.url,
                        binary.integrity,
                        destination=self.gen.data_root,
                        only_arches=[binary.arch.flatpak],
                    )
            else:
                assert False, self.electron_ffmpeg

    def _handle_gulp_atom_electron(self, package: Package) -> None:
        # Versions after 1.22.0 use @electron/get and don't need this
        if SemVer.parse(package.version) <= SemVer.parse('1.22.0'):
            cache_path = (
                self.gen.data_root / 'tmp' / 'gulp-electron-cache' / 'atom' / 'electron'
            )
            self.gen.add_command(f'mkdir -p "{cache_path.parent}"')
            self.gen.add_command(f'ln -sfTr "{self.electron_cache_dir}" "{cache_path}"')

    async def _handle_electron_headers(self, package: Package) -> None:
        node_headers = NodeHeaders.with_defaults(
            runtime='electron', target=package.version
        )
        if self.xdg_layout:
            node_gyp_headers_dir = (
                self.gen.data_root / 'cache' / 'node-gyp' / package.version
            )
        else:
            node_gyp_headers_dir = self.gen.data_root / 'node-gyp' / 'electron-current'
        await self.generate_node_headers(node_headers, dest=node_gyp_headers_dir)

    async def _get_chromedriver_binary_version(self, package: Package) -> str:
        # Note: node-chromedriver seems to not have tagged all releases on GitHub, so
        # just use unpkg instead.
        url = urllib.parse.urljoin(
            _NPM_MIRROR, f'chromedriver@{package.version}/lib/chromedriver'
        )
        js = await Requests.instance.read_all(url, cachable=True)
        # XXX: a tad ugly
        match = re.search(r"exports\.version = '([^']+)'", js.decode())
        assert (
            match is not None
        ), f'Failed to get ChromeDriver binary version from {url}'
        return match.group(1)

    async def _handle_electron_chromedriver(self, package: Package) -> None:
        manager = await ElectronBinaryManager.for_version(package.version)
        self._add_electron_cache_downloads(manager, 'chromedriver')

    async def _handle_node_chromedriver(self, package: Package) -> None:
        version = await self._get_chromedriver_binary_version(package)
        destination = self.gen.data_root / 'chromedriver'

        if self.node_chromedriver_from_electron is not None:
            manager = await ElectronBinaryManager.for_version(
                self.node_chromedriver_from_electron
            )

            for binary in manager.find_binaries('chromedriver'):
                assert binary.arch is not None
                self.gen.add_archive_source(
                    binary.url,
                    binary.integrity,
                    destination=destination,
                    only_arches=[binary.arch.flatpak],
                )
        else:
            url = (
                f'https://chromedriver.storage.googleapis.com/{version}/'
                'chromedriver_linux64.zip'
            )
            metadata = await RemoteUrlMetadata.get(url, cachable=True)

            self.gen.add_archive_source(
                url,
                metadata.integrity,
                destination=destination,
                only_arches=['x86_64'],
            )

    async def _add_nwjs_cache_downloads(
        self, version: str, flavor: str = 'normal'
    ) -> None:
        assert not version.startswith('v')
        nwjs_mirror = 'https://dl.nwjs.io'
        ffmpeg_dl_base = (
            'https://github.com/iteufel/nwjs-ffmpeg-prebuilt/releases/download'
        )

        if self.nwjs_node_headers:
            headers_dl_url = f'{nwjs_mirror}/v{version}/nw-headers-v{version}.tar.gz'
            headers_dest = self.gen.data_root / 'node-gyp' / 'nwjs-current'
            headers_metadata = await RemoteUrlMetadata.get(
                headers_dl_url, cachable=True
            )
            self.gen.add_archive_source(
                headers_dl_url,
                headers_metadata.integrity,
                destination=headers_dest,
            )

        if flavor == 'normal':
            filename_base = 'nwjs'
        else:
            filename_base = f'nwjs-{flavor}'

        destdir = self.gen.data_root / 'nwjs-cache'
        nwjs_arch_map = [
            ('x86_64', 'linux-x64', 'linux64'),
            ('i386', 'linux-ia32', 'linux32'),
        ]
        for flatpak_arch, nwjs_arch, platform in nwjs_arch_map:
            filename = f'{filename_base}-v{version}-{nwjs_arch}.tar.gz'
            dl_url = f'{nwjs_mirror}/v{version}/{filename}'
            metadata = await RemoteUrlMetadata.get(dl_url, cachable=True)
            dest = destdir / f'{version}-{flavor}' / platform

            self.gen.add_archive_source(
                dl_url,
                metadata.integrity,
                destination=dest,
                only_arches=[flatpak_arch],
            )

            if self.nwjs_ffmpeg:
                ffmpeg_dl_url = f'{ffmpeg_dl_base}/{version}/{version}-{nwjs_arch}.zip'
                ffmpeg_metadata = await RemoteUrlMetadata.get(
                    ffmpeg_dl_url, cachable=True
                )
                self.gen.add_archive_source(
                    ffmpeg_dl_url,
                    ffmpeg_metadata.integrity,
                    destination=dest,
                    strip_components=0,
                    only_arches=[flatpak_arch],
                )

    async def _handle_nw_builder(self, package: Package) -> None:
        if self.nwjs_version:
            version = self.nwjs_version
        else:
            versions_json = json.loads(
                await Requests.instance.read_all(
                    'https://nwjs.io/versions.json', cachable=False
                )
            )
            version = versions_json['latest'].lstrip('v')
        await self._add_nwjs_cache_downloads(version)
        self.gen.add_data_source(
            version, destination=self.gen.data_root / 'nwjs-version'
        )

    async def _handle_dugite_native(self, package: Package) -> None:
        dl_json_url = urllib.parse.urljoin(
            _NPM_MIRROR,
            f'{package.name}@{package.version}/script/embedded-git.json',
        )
        dl_json = json.loads(
            await Requests.instance.read_all(dl_json_url, cachable=True)
        )
        dugite_arch_map = {
            'x86_64': 'linux-x64',
        }
        destdir = self.gen.data_root / 'tmp'
        for arch, dugite_arch in dugite_arch_map.items():
            url = dl_json[dugite_arch]['url']
            filename = dl_json[dugite_arch]['name']
            integrity = Integrity(
                algorithm='sha256', digest=dl_json[dugite_arch]['checksum']
            )

            self.gen.add_url_source(
                url,
                integrity,
                destination=destdir / filename,
                only_arches=[arch],
            )

    async def _handle_ripgrep_prebuilt(self, package: Package) -> None:
        async def get_ripgrep_tag(version: str) -> str:
            url = f'https://github.com/microsoft/vscode-ripgrep/raw/v{version}/lib/postinstall.js'
            tag_re = re.compile(r"VERSION\s+=\s+'(v[\d.-]+)';")
            resp = await Requests.instance.read_all(url, cachable=True)
            match = tag_re.search(resp.decode())
            assert match is not None
            return match.group(1)

        tag = await get_ripgrep_tag(package.version)
        ripgrep_arch_map = {
            'x86_64': 'x86_64-unknown-linux-musl',
            'i386': 'i686-unknown-linux-musl',
            'arm': 'arm-unknown-linux-gnueabihf',
            'aarch64': 'aarch64-unknown-linux-gnu',
        }
        destdir = self.gen.data_root / 'tmp' / f'vscode-ripgrep-cache-{package.version}'
        for arch, ripgrep_arch in ripgrep_arch_map.items():
            filename = f'ripgrep-{tag}-{ripgrep_arch}.tar.gz'
            url = f'https://github.com/microsoft/ripgrep-prebuilt/releases/download/{tag}/{filename}'
            metadata = await RemoteUrlMetadata.get(url, cachable=True)
            self.gen.add_url_source(
                url,
                metadata.integrity,
                destination=destdir / filename,
                only_arches=[arch],
            )

    async def _handle_playwright(self, package: Package) -> None:
        base_url = f'https://github.com/microsoft/playwright/raw/v{package.version}/'
        if SemVer.parse(package.version) >= SemVer.parse('1.16.0'):
            browsers_json_url = base_url + 'packages/playwright-core/browsers.json'
        else:
            browsers_json_url = base_url + 'browsers.json'
        browsers_json = json.loads(
            await Requests.instance.read_all(browsers_json_url, cachable=True)
        )
        for browser in browsers_json['browsers']:
            if not browser.get('installByDefault', True):
                continue
            name = browser['name']
            revision = int(browser['revision'])

            if name == 'chromium':
                # Revision number scheme was changed from Chromium revisions to incrementing
                # integers above 1000; now it's same as for other browsers
                if (
                    SemVer.parse(package.version) < SemVer.parse('1.21.0')
                    and revision < 792639
                ):
                    url_tp = 'https://storage.googleapis.com/chromium-browser-snapshots/Linux_x64/%d/%s'
                    dl_file = 'chrome-linux.zip'
                else:
                    url_tp = 'https://playwright.azureedge.net/builds/chromium/%d/%s'
                    dl_file = 'chromium-linux.zip'
            elif name == 'firefox':
                url_tp = 'https://playwright.azureedge.net/builds/firefox/%d/%s'
                if revision < 1140:
                    dl_file = 'firefox-linux.zip'
                else:
                    dl_file = 'firefox-ubuntu-22.04.zip'
            elif name == 'webkit':
                url_tp = 'https://playwright.azureedge.net/builds/webkit/%d/%s'
                if revision < 1317:
                    dl_file = 'minibrowser-gtk-wpe.zip'
                else:
                    dl_file = 'webkit-ubuntu-20.04.zip'
            elif name == 'ffmpeg':
                url_tp = 'https://playwright.azureedge.net/builds/ffmpeg/%d/%s'
                dl_file = 'ffmpeg-linux.zip'
            else:
                raise ValueError(f'Unknown playwright browser {name}')

            dl_url = url_tp % (revision, dl_file)
            metadata = await RemoteUrlMetadata.get(dl_url, cachable=True)
            destdir = (
                self.gen.data_root / 'cache' / 'ms-playwright' / f'{name}-{revision}'
            )
            self.gen.add_archive_source(
                dl_url,
                metadata.integrity,
                destination=destdir,
                strip_components=0,
            )
            # Arbitrary string here; flatpak-builder segfaults on empty data: url
            self.gen.add_data_source(
                'flatpak-node-cache',
                destination=destdir / 'INSTALLATION_COMPLETE',
            )

    async def _handle_esbuild(self, package: Package) -> None:
        pkg_names = [
            ('x86_64', '@esbuild/linux-x64', 'esbuild-linux-64'),
            ('i386', '@esbuild/linux-ia32', 'esbuild-linux-32'),
            ('arm', '@esbuild/linux-arm', 'esbuild-linux-arm'),
            ('aarch64', '@esbuild/linux-arm64', 'esbuild-linux-arm64'),
        ]

        pkg_name_is_scoped = SemVer.parse(package.version) >= SemVer.parse('0.16.0')

        for flatpak_arch, new_pkg_name, old_pkg_name in pkg_names:
            pkg_name = new_pkg_name if pkg_name_is_scoped else old_pkg_name
            data_url = f'https://registry.npmjs.org/{pkg_name}/{package.version}'
            registry_data = json.loads(await Requests.instance.read_all(data_url))

            dl_url = registry_data['dist']['tarball']
            integrity = Integrity.parse(registry_data['dist']['integrity'])

            cache_dst = self.gen.data_root / 'cache' / 'esbuild'
            archive_dst = cache_dst / '.package' / f'{pkg_name}@{package.version}'
            bin_src = archive_dst / 'bin' / 'esbuild'
            bin_dst = cache_dst / 'bin' / f'{pkg_name}@{package.version}'

            self.gen.add_archive_source(
                dl_url,
                integrity,
                destination=archive_dst,
                only_arches=[flatpak_arch],
                strip_components=1,
            )

            cmd = [
                f'mkdir -p "{bin_dst.parent.relative_to(cache_dst)}"',
                f'cp "{bin_src.relative_to(cache_dst)}" "{bin_dst.relative_to(cache_dst)}"',
                f'ln -sf "{bin_dst.name}" "bin/esbuild-current"',
            ]
            self.gen.add_shell_source(
                cmd, only_arches=[flatpak_arch], destination=cache_dst
            )

    def _handle_electron_builder(self, package: Package) -> None:
        destination = self.gen.data_root / 'electron-builder-arch-args.sh'

        script: List[str] = []
        script.append('case "$FLATPAK_ARCH" in')

        for (
            electron_arch,
            flatpak_arch,
        ) in ElectronBinaryManager.ELECTRON_ARCHES_TO_FLATPAK.items():
            script.append(f'"{flatpak_arch}")')
            script.append(f'  export ELECTRON_BUILDER_ARCH_ARGS="--{electron_arch}"')
            script.append('  ;;')

        script.append('esac')

        self.gen.add_script_source(script, destination)

    async def generate_node_headers(
        self, node_headers: NodeHeaders, dest: Optional[Path] = None
    ) -> None:
        url = node_headers.url
        install_version = node_headers.install_version
        if dest is None:
            dest = self.gyp_dir / node_headers.target
        metadata = await RemoteUrlMetadata.get(url, cachable=True)
        self.gen.add_archive_source(url, metadata.integrity, destination=dest)
        self.gen.add_data_source(install_version, destination=dest / 'installVersion')

    async def generate_special_sources(self, package: Package) -> None:
        if isinstance(Requests.instance, StubRequests):
            # This is going to crash and burn.
            return

        if package.name == 'electron':
            await self._handle_electron(package)
            if self.electron_node_headers:
                await self._handle_electron_headers(package)
        elif package.name == 'electron-chromedriver':
            await self._handle_electron_chromedriver(package)
        elif package.name == 'chromedriver':
            await self._handle_node_chromedriver(package)
        elif package.name == 'electron-builder':
            self._handle_electron_builder(package)
        elif package.name == 'gulp-atom-electron':
            self._handle_gulp_atom_electron(package)
        elif package.name == 'nw-builder':
            await self._handle_nw_builder(package)
        elif package.name in {'dugite', '@shiftkey/dugite'}:
            await self._handle_dugite_native(package)
        elif package.name in {'vscode-ripgrep', '@vscode/ripgrep'}:
            await self._handle_ripgrep_prebuilt(package)
        elif package.name == 'playwright':
            await self._handle_playwright(package)
        elif package.name == 'esbuild':
            await self._handle_esbuild(package)
