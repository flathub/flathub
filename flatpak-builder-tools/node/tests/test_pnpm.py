import hashlib
import json
from pathlib import Path

import pytest
from conftest import RequestsController

from flatpak_node_generator.integrity import Integrity
from flatpak_node_generator.manifest import ManifestGenerator
from flatpak_node_generator.package import (
    GitSource,
    LocalSource,
    Lockfile,
    Package,
    ResolvedSource,
)
from flatpak_node_generator.providers.pnpm import (
    PnpmLockfileProvider,
    PnpmModuleProvider,
)
from flatpak_node_generator.providers.special import SpecialSourceProvider

TEST_LOCKFILE_V9 = """
lockfileVersion: '9.0'

settings:
  autoInstallPeers: true
  excludeLinksFromLockfile: false

importers:
  .:
    dependencies:
      is-odd:
        specifier: ^3.0.1
        version: 3.0.1

packages:
  is-number@6.0.0:
    resolution: {integrity: sha512-Wu1VHeILBK8KAWJUAiSZQX94GmOE45Rg6/538fKwiloUu21KncEkYGPqob2oSZ5mUT73vLGrHQjKw3KMPwfDzg==}
    engines: {node: '>=0.10.0'}

  is-odd@3.0.1:
    resolution: {integrity: sha512-CQpnWPrDwmP1+SMHXZhtLtJv90yiyVfluGsX5iNCVkrhQtU3TQHsUWPG9wkdk9Lgd5yNpAg9jQEo90CBaXgWMA==}
    engines: {node: '>=4'}

  '@babel/core@7.23.0':
    resolution: {integrity: sha256-dGVzdA==}
    engines: {node: '>=6.9.0'}

snapshots:
  is-number@6.0.0: {}

  is-odd@3.0.1:
    dependencies:
      is-number: 6.0.0

  '@babel/core@7.23.0': {}
"""

TEST_LOCKFILE_V6 = """
lockfileVersion: '6.0'

dependencies:
  is-odd:
    specifier: ^3.0.1
    version: 3.0.1

devDependencies:
  is-number:
    specifier: ^6.0.0
    version: 6.0.0

packages:
  /is-number@6.0.0:
    resolution: {integrity: sha512-Wu1VHeILBK8KAWJUAiSZQX94GmOE45Rg6/538fKwiloUu21KncEkYGPqob2oSZ5mUT73vLGrHQjKw3KMPwfDzg==}
    engines: {node: '>=0.10.0'}
    dev: true

  /is-odd@3.0.1:
    resolution: {integrity: sha512-CQpnWPrDwmP1+SMHXZhtLtJv90yiyVfluGsX5iNCVkrhQtU3TQHsUWPG9wkdk9Lgd5yNpAg9jQEo90CBaXgWMA==}
    engines: {node: '>=4'}
    dev: false

  /next@14.0.5(react-dom@18.2.0)(react@18.2.0):
    resolution: {integrity: sha256-dGVzdA==}
    engines: {node: '>=18'}
    dev: false
"""


def test_lockfile_v9(tmp_path: Path) -> None:
    provider = PnpmLockfileProvider(
        PnpmLockfileProvider.Options(
            no_devel=False,
            registry='https://registry.npmjs.org',
            store_version=None,
        )
    )

    lockfile = Lockfile(tmp_path / 'pnpm-lock.yaml', 9, store_version='v10')
    lockfile.path.write_text(TEST_LOCKFILE_V9)

    packages = list(provider.process_lockfile(lockfile.path))

    assert packages == [
        Package(
            lockfile=lockfile,
            name='is-number',
            version='6.0.0',
            source=ResolvedSource(
                resolved='https://registry.npmjs.org/is-number/-/is-number-6.0.0.tgz',
                integrity=Integrity(
                    'sha512',
                    '5aed551de20b04af0a016254022499417f781a6384e39460ebfe77f1f2b08a5a14bb6d4a9dc1246063eaa1bda8499e66513ef7bcb1ab1d08cac3728c3f07c3ce',
                ),
            ),
        ),
        Package(
            lockfile=lockfile,
            name='is-odd',
            version='3.0.1',
            source=ResolvedSource(
                resolved='https://registry.npmjs.org/is-odd/-/is-odd-3.0.1.tgz',
                integrity=Integrity(
                    'sha512',
                    '090a6758fac3c263f5f923075d986d2ed26ff74ca2c957e5b86b17e62342564ae142d5374d01ec5163c6f7091d93d2e0779c8da4083d8d0128f7408169781630',
                ),
            ),
        ),
        Package(
            lockfile=lockfile,
            name='@babel/core',
            version='7.23.0',
            source=ResolvedSource(
                resolved='https://registry.npmjs.org/@babel/core/-/core-7.23.0.tgz',
                integrity=Integrity(
                    'sha256',
                    '74657374',
                ),
            ),
        ),
    ]


def test_lockfile_v6(tmp_path: Path) -> None:
    provider = PnpmLockfileProvider(
        PnpmLockfileProvider.Options(
            no_devel=False,
            registry='https://registry.npmjs.org',
            store_version=None,
        )
    )

    lockfile = Lockfile(tmp_path / 'pnpm-lock.yaml', 6, store_version='v3')
    lockfile.path.write_text(TEST_LOCKFILE_V6)

    packages = list(provider.process_lockfile(lockfile.path))

    assert packages == [
        Package(
            lockfile=lockfile,
            name='is-number',
            version='6.0.0',
            source=ResolvedSource(
                resolved='https://registry.npmjs.org/is-number/-/is-number-6.0.0.tgz',
                integrity=Integrity(
                    'sha512',
                    '5aed551de20b04af0a016254022499417f781a6384e39460ebfe77f1f2b08a5a14bb6d4a9dc1246063eaa1bda8499e66513ef7bcb1ab1d08cac3728c3f07c3ce',
                ),
            ),
        ),
        Package(
            lockfile=lockfile,
            name='is-odd',
            version='3.0.1',
            source=ResolvedSource(
                resolved='https://registry.npmjs.org/is-odd/-/is-odd-3.0.1.tgz',
                integrity=Integrity(
                    'sha512',
                    '090a6758fac3c263f5f923075d986d2ed26ff74ca2c957e5b86b17e62342564ae142d5374d01ec5163c6f7091d93d2e0779c8da4083d8d0128f7408169781630',
                ),
            ),
        ),
        Package(
            lockfile=lockfile,
            name='next',
            version='14.0.5',
            source=ResolvedSource(
                resolved='https://registry.npmjs.org/next/-/next-14.0.5.tgz',
                integrity=Integrity(
                    'sha256',
                    '74657374',
                ),
            ),
        ),
    ]


def test_lockfile_v6_no_devel(tmp_path: Path) -> None:
    provider = PnpmLockfileProvider(
        PnpmLockfileProvider.Options(
            no_devel=True,
            registry='https://registry.npmjs.org',
            store_version=None,
        )
    )

    lockfile = Lockfile(tmp_path / 'pnpm-lock.yaml', 6)
    lockfile.path.write_text(TEST_LOCKFILE_V6)

    packages = list(provider.process_lockfile(lockfile.path))

    names = [p.name for p in packages]
    assert 'is-number' not in names
    assert 'is-odd' in names
    assert 'next' in names


def test_lockfile_v5_rejected(tmp_path: Path) -> None:
    provider = PnpmLockfileProvider(
        PnpmLockfileProvider.Options(
            no_devel=False,
            registry='https://registry.npmjs.org',
            store_version=None,
        )
    )

    lockfile_path = tmp_path / 'pnpm-lock.yaml'
    lockfile_path.write_text(
        'lockfileVersion: 5.4\npackages:\n'
        '  /is-odd/3.0.1:\n'
        '    resolution: {integrity: sha256-dGVzdA==}\n'
    )

    with pytest.raises(ValueError, match='unsupported lockfileVersion 5.4'):
        list(provider.process_lockfile(lockfile_path))


def test_lockfile_unsupported_version_rejected(tmp_path: Path) -> None:
    provider = PnpmLockfileProvider(
        PnpmLockfileProvider.Options(
            no_devel=False,
            registry='https://registry.npmjs.org',
            store_version=None,
        )
    )

    lockfile_path = tmp_path / 'pnpm-lock.yaml'
    lockfile_path.write_text(
        'lockfileVersion: 42\npackages:\n'
        '  foo@1.0.0:\n'
        '    resolution: {integrity: sha256-dGVzdA==}\n'
    )

    with pytest.raises(ValueError, match='unsupported lockfileVersion 42'):
        list(provider.process_lockfile(lockfile_path))


def test_lockfile_v9_no_devel_warns(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    provider = PnpmLockfileProvider(
        PnpmLockfileProvider.Options(
            no_devel=True,
            registry='https://registry.npmjs.org',
            store_version=None,
        )
    )

    lockfile = Lockfile(tmp_path / 'pnpm-lock.yaml', 9)
    lockfile.path.write_text(TEST_LOCKFILE_V9)

    packages = list(provider.process_lockfile(lockfile.path))

    captured = capsys.readouterr()
    assert '--no-devel is not yet supported for pnpm lockfile v9' in captured.err
    assert len(packages) == 3


TEST_LOCKFILE_V9_GIT_AND_LOCAL = """
lockfileVersion: '9.0'

settings:
  autoInstallPeers: true
  excludeLinksFromLockfile: false

importers:
  .:
    dependencies:
      my-git-pkg:
        specifier: github:user/repo#abc123
        version: github.com/user/repo/abc123
      my-local-pkg:
        specifier: file:../local-pkg
        version: file:../local-pkg

packages:
  my-git-pkg@github.com/user/repo/abc123:
    resolution: {type: git, repo: https://github.com/user/repo, commit: abc123def456}

  my-local-pkg@file:../local-pkg:
    resolution: {directory: ../local-pkg}

snapshots:
  my-git-pkg@github.com/user/repo/abc123: {}
  my-local-pkg@file:../local-pkg: {}
"""


def test_lockfile_v9_git_and_local(tmp_path: Path) -> None:
    provider = PnpmLockfileProvider(
        PnpmLockfileProvider.Options(
            no_devel=False,
            registry='https://registry.npmjs.org',
            store_version=None,
        )
    )

    lockfile = Lockfile(tmp_path / 'pnpm-lock.yaml', 9, store_version='v10')
    lockfile.path.write_text(TEST_LOCKFILE_V9_GIT_AND_LOCAL)

    packages = list(provider.process_lockfile(lockfile.path))

    assert packages == [
        Package(
            lockfile=lockfile,
            name='my-git-pkg',
            version='github.com/user/repo/abc123',
            source=GitSource(
                original='git+https://github.com/user/repo#abc123def456',
                url='https://github.com/user/repo',
                commit='abc123def456',
                from_=None,
            ),
        ),
        Package(
            lockfile=lockfile,
            name='my-local-pkg',
            version='file:../local-pkg',
            source=LocalSource(path='../local-pkg'),
        ),
    ]


def test_pnpm_module_provider_tarball_url(tmp_path: Path) -> None:
    gen = ManifestGenerator()
    special = SpecialSourceProvider(
        gen,
        SpecialSourceProvider.Options(
            node_chromedriver_from_electron=None,
            electron_ffmpeg=None,
            electron_node_headers=False,
            nwjs_version=None,
            nwjs_node_headers=False,
            nwjs_ffmpeg=False,
            xdg_layout=True,
            node_sdk_extension=None,
        ),
    )
    provider = PnpmModuleProvider(gen, special, tmp_path)

    provider._store_version = 'v3'
    provider._tarballs = [
        PnpmModuleProvider._TarballInfo(
            tarball_name='normal-pkg-1.0.0.tgz',
            name='normal-pkg',
            version='1.0.0',
            integrity=Integrity('sha512', 'abc123def456ab'),
        ),
        PnpmModuleProvider._TarballInfo(
            tarball_name='url-pkg-http-123.tgz',
            name='url-pkg',
            version='http://example.com/url-pkg.tgz',
            integrity=Integrity('sha512', 'fedcba65401234'),
        ),
        PnpmModuleProvider._TarballInfo(
            tarball_name='url-pkg-https-123.tgz',
            name='url-pkg-2',
            version='https://example.com/url-pkg-2.tgz',
            integrity=Integrity('sha512', '99999999'),
        ),
    ]

    provider._add_store_population_script()

    # Manifest data source should have been added to gen._sources
    manifest_source_dict = next(
        dict(s)
        for s in gen._sources
        if dict(s).get('dest-filename') == 'pnpm-manifest.json'
    )
    assert manifest_source_dict is not None

    manifest_data = json.loads(manifest_source_dict['contents'])
    packages = manifest_data['packages']

    # Check each package based on original tarballs for correct handling
    for tarball in provider._tarballs:
        pkg = packages[tarball.tarball_name]
        assert pkg['version'] == tarball.version
        expected_integrity = (
            f'{tarball.integrity.algorithm}-{tarball.integrity.to_base64()}'
        )
        assert f'{pkg["integrity_algo"]}-{pkg["integrity"]}' == expected_integrity
        if tarball.version.startswith(('http://', 'https://')):
            assert pkg['tarball_url'] == tarball.version
        else:
            assert 'tarball_url' not in pkg


@pytest.mark.asyncio
async def test_pnpm_module_provider_missing_integrity(
    tmp_path: Path, requests: RequestsController
) -> None:

    gen = ManifestGenerator()
    special = SpecialSourceProvider(
        gen,
        SpecialSourceProvider.Options(
            node_chromedriver_from_electron=None,
            electron_ffmpeg=None,
            electron_node_headers=False,
            nwjs_version=None,
            nwjs_node_headers=False,
            nwjs_ffmpeg=False,
            xdg_layout=True,
            node_sdk_extension=None,
        ),
    )

    provider = PnpmModuleProvider(gen, special, tmp_path)

    lockfile = Lockfile(tmp_path / 'pnpm-lock.yaml', 9, store_version='v3')

    test_data = b'dummy tarball content'
    test_digest = hashlib.sha256(test_data).hexdigest()
    expected_integrity = Integrity('sha256', test_digest)

    requests.server.expect_oneshot_request(
        '/test-pkg-1.0.0.tgz', 'GET'
    ).respond_with_data(test_data)

    source = ResolvedSource(
        resolved=requests.url_for('/test-pkg-1.0.0.tgz'),
        integrity=None,
    )

    pkg = Package(
        lockfile=lockfile,
        name='test-pkg',
        version='1.0.0',
        source=source,
    )

    await provider.generate_package(pkg)

    # Assert tarball was added with computed integrity
    assert len(provider._tarballs) == 1
    assert provider._tarballs[0].integrity == expected_integrity

    # Assert it was added to manifest generator with the right integrity
    tarball_source = next(
        dict(s) for s in gen._sources if dict(s).get('url') == source.resolved
    )
    assert tarball_source['sha256'] == expected_integrity.digest
