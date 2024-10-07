from typing import Dict, Iterator, NamedTuple, Optional

import hashlib
import os.path
import urllib.parse

from .integrity import Integrity
from .package import SemVer
from .requests import Requests


class ElectronBinaryManager:
    class Arch(NamedTuple):
        electron: str
        flatpak: str

    class Binary(NamedTuple):
        filename: str
        url: str
        integrity: Integrity

        arch: Optional['ElectronBinaryManager.Arch'] = None

        @property
        def url_hash(self) -> str:
            url = urllib.parse.urlparse(self.url)
            url_dir = urllib.parse.urlunparse(
                url._replace(path=os.path.dirname(url.path))
            )
            return hashlib.sha256(url_dir.encode()).hexdigest()

    ELECTRON_ARCHES_TO_FLATPAK = {
        'ia32': 'i386',
        'x64': 'x86_64',
        'armv7l': 'arm',
        'arm64': 'aarch64',
    }

    INTEGRITY_BASE_FILENAME = 'SHASUMS256.txt'

    def __init__(
        self, version: str, base_url: str, integrities: Dict[str, Integrity]
    ) -> None:
        self.version = version
        self.base_url = base_url
        self.integrities = integrities

    def child_url(self, child: str) -> str:
        return f'{self.base_url}/{child}'

    def find_binaries(self, binary: str) -> Iterator['ElectronBinaryManager.Binary']:
        for electron_arch, flatpak_arch in self.ELECTRON_ARCHES_TO_FLATPAK.items():
            # Electron v19+ drop linux-ia32 support.
            if (
                SemVer.parse(self.version) >= SemVer.parse('19.0.0')
                and electron_arch == 'ia32'
            ):
                continue

            binary_filename = f'{binary}-v{self.version}-linux-{electron_arch}.zip'
            binary_url = self.child_url(binary_filename)

            arch = ElectronBinaryManager.Arch(
                electron=electron_arch, flatpak=flatpak_arch
            )
            yield ElectronBinaryManager.Binary(
                filename=binary_filename,
                url=binary_url,
                integrity=self.integrities[binary_filename],
                arch=arch,
            )

    @property
    def integrity_file(self) -> 'ElectronBinaryManager.Binary':
        return ElectronBinaryManager.Binary(
            filename=f'SHASUMS256.txt-{self.version}',
            url=self.child_url(self.INTEGRITY_BASE_FILENAME),
            integrity=self.integrities[self.INTEGRITY_BASE_FILENAME],
        )

    @staticmethod
    async def for_version(
        version: str, *, base_url: Optional[str] = None
    ) -> 'ElectronBinaryManager':
        if base_url is None:
            base_url = (
                f'https://github.com/electron/electron/releases/download/v{version}'
            )

        integrity_url = f'{base_url}/{ElectronBinaryManager.INTEGRITY_BASE_FILENAME}'
        integrity_data = (
            await Requests.instance.read_all(integrity_url, cachable=True)
        ).decode()

        integrities: Dict[str, Integrity] = {}
        for line in integrity_data.splitlines():
            digest, star_filename = line.split()
            filename = star_filename.strip('*')
            integrities[filename] = Integrity(algorithm='sha256', digest=digest)

        integrities[ElectronBinaryManager.INTEGRITY_BASE_FILENAME] = Integrity.generate(
            integrity_data
        )

        return ElectronBinaryManager(
            version=version, base_url=base_url, integrities=integrities
        )
