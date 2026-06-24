from __future__ import annotations

import json
import os
import platform
import struct
from typing import NamedTuple

NODE_GYP_INSTALL_VERSION = '11'


class NodeHeaders(NamedTuple):
    target: str
    runtime: str
    disturl: str

    @classmethod
    def with_defaults(
        cls,
        target: str,
        runtime: str | None = None,
        disturl: str | None = None,
    ) -> NodeHeaders:
        if runtime is None:
            runtime = 'node'
        if disturl is None:
            if runtime == 'node':
                disturl = 'http://nodejs.org/dist'
            elif runtime == 'electron':
                disturl = 'https://www.electronjs.org/headers'
            else:
                raise ValueError(
                    f"Can't guess `disturl` for {runtime} version {target}"
                )
        return cls(target, runtime, disturl)

    @property
    def url(self) -> str:
        # TODO it may be better to retrieve urls from disturl/index.json
        return f'{self.disturl}/v{self.target}/node-v{self.target}-headers.tar.gz'

    @staticmethod
    def _get_flatpak_arch() -> str:
        machine = platform.machine().lower()
        is_32bit = struct.calcsize('P') * 8 == 32

        if machine in ('x86_64', 'amd64'):
            return 'i386' if is_32bit else 'x86_64'

        if machine in ('i386', 'i486', 'i586', 'i686'):
            return 'i386'

        if machine == 'aarch64':
            return 'aarch64'

        if machine.startswith('arm'):
            return 'arm'

        return machine

    @staticmethod
    def _find_node_gyp_path(sdk_extension: str) -> str | None:
        try:
            ext_id, version = sdk_extension.split('//')
        except ValueError:
            return None

        flatpak_user_dir = os.environ.get('FLATPAK_USER_DIR')
        if flatpak_user_dir:
            search_roots = [flatpak_user_dir]
        else:
            xdg_data_home = os.environ.get(
                'XDG_DATA_HOME',
                os.path.expanduser('~/.local/share'),
            )
            search_roots = [
                os.path.join(xdg_data_home, 'flatpak'),
                '/var/lib/flatpak',
            ]

        arch = NodeHeaders._get_flatpak_arch()

        for root in search_roots:
            candidate = os.path.join(
                root,
                'runtime',
                ext_id,
                arch,
                version,
                'active',
                'files',
                'lib',
                'node_modules',
                'npm',
                'node_modules',
                'node-gyp',
                'package.json',
            )

            if os.path.isfile(candidate):
                return candidate

        return None

    def install_version(self, sdk_extension: str | None = None) -> str:
        if sdk_extension is None:
            print(
                f"\nNo Node SDK extension supplied, using node-gyp installVersion '{NODE_GYP_INSTALL_VERSION}'"
            )
            return NODE_GYP_INSTALL_VERSION

        pkg_path = self._find_node_gyp_path(sdk_extension)

        if pkg_path is None:
            print(
                f'\nFailed to detect node-gyp installVersion from Node SDK extension '
                f"'{sdk_extension}', using '{NODE_GYP_INSTALL_VERSION}'"
            )
            return NODE_GYP_INSTALL_VERSION

        try:
            with open(pkg_path, encoding='utf-8') as f:
                data = json.load(f)
                version = str(data['installVersion'])
                print(
                    f"\nUsing node-gyp installVersion '{version}' "
                    f"from SDK extension '{sdk_extension}'"
                )
                return version
        except json.JSONDecodeError as e:
            print(
                f'\nFailed to read node-gyp installVersion from Node SDK extension '
                f"'{sdk_extension}', using '{NODE_GYP_INSTALL_VERSION}': {e}"
            )
            return NODE_GYP_INSTALL_VERSION
