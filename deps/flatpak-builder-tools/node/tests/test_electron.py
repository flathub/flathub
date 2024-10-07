from typing import List

from conftest import RequestsController
from flatpak_node_generator.electron import ElectronBinaryManager
from flatpak_node_generator.integrity import Integrity

VERSION = '18.0.0'

INTEGRITY_DATA = """
b3c088672deab866b0da85ec02338a3e2c541332a946814b7cd09e9a16cd41d6 *electron-v18.0.0-linux-arm64.zip
991c54c875102c1440f45a97509693003c8c5c3f1c69123e0913236396202dba *electron-v18.0.0-linux-armv7l.zip
995fd56b0c03abcac575b920092876b85185d3e13d63b2007e8e49f395f64061 *electron-v18.0.0-linux-ia32.zip
115737fb1e6759bcb9822e642f9457c2ee1ae7c3413dc3f356bf5d15576fec4d *electron-v18.0.0-linux-x64.zip
3f020561f8cd30e0fb51b82686fbb3b2a81a92990e9dd009234f19fcac48a2ce *electron.d.ts
9d80371fc2a146f6671aeb70dea36638400569fd130f2773a2f79b83b3e2c127 *ffmpeg-v18.0.0-linux-arm64.zip
60cf19bf219cb7ccf190f86e1bc6e8cc0ed0d7e9889faffc0138f443aaf17baf *ffmpeg-v18.0.0-linux-armv7l.zip
af80bceef443bf01bced73eea3e8f700cca1f20560c97a0b588b49577d0a9141 *ffmpeg-v18.0.0-linux-ia32.zip
1d7c771bdfd727b7707cf7635c8b0fad66c89d03610b9bac793429ae3ddb458c *ffmpeg-v18.0.0-linux-x64.zip
""".strip()

INTEGRITY_SHA256 = '0cba8a5d7280d8cf264c0b8399e93e4cc75d8e18c3b89ae351e443d2c4437f47'


def _expect_integrity_request(requests: RequestsController) -> None:
    requests.server.expect_oneshot_request('/SHASUMS256.txt', 'GET').respond_with_data(
        INTEGRITY_DATA
    )


def _list_binaries(
    manager: ElectronBinaryManager,
    binary: str,
) -> List[ElectronBinaryManager.Binary]:
    return list(sorted(manager.find_binaries(binary), key=lambda b: b.filename))


async def test_integrity_file(requests: RequestsController) -> None:
    _expect_integrity_request(requests)
    manager = await ElectronBinaryManager.for_version(
        VERSION, base_url=requests.url.rstrip('/')
    )

    assert manager.integrity_file == ElectronBinaryManager.Binary(
        filename=f'SHASUMS256.txt-{VERSION}',
        url=requests.server.url_for('SHASUMS256.txt'),
        integrity=Integrity('sha256', INTEGRITY_SHA256),
        arch=None,
    )


async def test_electron_binaries(requests: RequestsController) -> None:
    _expect_integrity_request(requests)
    manager = await ElectronBinaryManager.for_version(
        VERSION, base_url=requests.url.rstrip('/')
    )

    assert _list_binaries(manager, 'electron') == [
        ElectronBinaryManager.Binary(
            filename='electron-v18.0.0-linux-arm64.zip',
            url=requests.server.url_for('electron-v18.0.0-linux-arm64.zip'),
            integrity=Integrity(
                'sha256',
                'b3c088672deab866b0da85ec02338a3e2c541332a946814b7cd09e9a16cd41d6',
            ),
            arch=ElectronBinaryManager.Arch(electron='arm64', flatpak='aarch64'),
        ),
        ElectronBinaryManager.Binary(
            filename='electron-v18.0.0-linux-armv7l.zip',
            url=requests.server.url_for('electron-v18.0.0-linux-armv7l.zip'),
            integrity=Integrity(
                'sha256',
                '991c54c875102c1440f45a97509693003c8c5c3f1c69123e0913236396202dba',
            ),
            arch=ElectronBinaryManager.Arch(electron='armv7l', flatpak='arm'),
        ),
        ElectronBinaryManager.Binary(
            filename='electron-v18.0.0-linux-ia32.zip',
            url=requests.server.url_for('electron-v18.0.0-linux-ia32.zip'),
            integrity=Integrity(
                'sha256',
                '995fd56b0c03abcac575b920092876b85185d3e13d63b2007e8e49f395f64061',
            ),
            arch=ElectronBinaryManager.Arch(electron='ia32', flatpak='i386'),
        ),
        ElectronBinaryManager.Binary(
            filename='electron-v18.0.0-linux-x64.zip',
            url=requests.server.url_for('electron-v18.0.0-linux-x64.zip'),
            integrity=Integrity(
                'sha256',
                '115737fb1e6759bcb9822e642f9457c2ee1ae7c3413dc3f356bf5d15576fec4d',
            ),
            arch=ElectronBinaryManager.Arch(electron='x64', flatpak='x86_64'),
        ),
    ]


async def test_ffmpeg_binaries(requests: RequestsController) -> None:
    _expect_integrity_request(requests)
    manager = await ElectronBinaryManager.for_version(
        VERSION, base_url=requests.url.rstrip('/')
    )

    assert _list_binaries(manager, 'ffmpeg') == [
        ElectronBinaryManager.Binary(
            filename='ffmpeg-v18.0.0-linux-arm64.zip',
            url=requests.server.url_for('ffmpeg-v18.0.0-linux-arm64.zip'),
            integrity=Integrity(
                'sha256',
                '9d80371fc2a146f6671aeb70dea36638400569fd130f2773a2f79b83b3e2c127',
            ),
            arch=ElectronBinaryManager.Arch(electron='arm64', flatpak='aarch64'),
        ),
        ElectronBinaryManager.Binary(
            filename='ffmpeg-v18.0.0-linux-armv7l.zip',
            url=requests.server.url_for('ffmpeg-v18.0.0-linux-armv7l.zip'),
            integrity=Integrity(
                'sha256',
                '60cf19bf219cb7ccf190f86e1bc6e8cc0ed0d7e9889faffc0138f443aaf17baf',
            ),
            arch=ElectronBinaryManager.Arch(electron='armv7l', flatpak='arm'),
        ),
        ElectronBinaryManager.Binary(
            filename='ffmpeg-v18.0.0-linux-ia32.zip',
            url=requests.server.url_for('ffmpeg-v18.0.0-linux-ia32.zip'),
            integrity=Integrity(
                'sha256',
                'af80bceef443bf01bced73eea3e8f700cca1f20560c97a0b588b49577d0a9141',
            ),
            arch=ElectronBinaryManager.Arch(electron='ia32', flatpak='i386'),
        ),
        ElectronBinaryManager.Binary(
            filename='ffmpeg-v18.0.0-linux-x64.zip',
            url=requests.server.url_for('ffmpeg-v18.0.0-linux-x64.zip'),
            integrity=Integrity(
                'sha256',
                '1d7c771bdfd727b7707cf7635c8b0fad66c89d03610b9bac793429ae3ddb458c',
            ),
            arch=ElectronBinaryManager.Arch(electron='x64', flatpak='x86_64'),
        ),
    ]
