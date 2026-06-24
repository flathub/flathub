import itertools
import shlex
from pathlib import Path

import pytest
from conftest import FlatpakBuilder, ProviderFactorySpec

from flatpak_node_generator.manifest import ManifestGenerator

pytestmark = pytest.mark.integration

TEST_SCRIPT = """
require("array-range");
require("is-empty-object");
require("is-number");
require("person-lib");
require("to-camel-case");
require("to-capital-case");
require("to-no-case");
require("to-space-case");
require("@flatpak-node-generator-tests/subdir").sayHello();
"""


async def test_minimal_git(
    flatpak_builder: FlatpakBuilder,
    provider_factory_spec: ProviderFactorySpec,
    node_version: int,
) -> None:
    with ManifestGenerator() as gen:
        await provider_factory_spec.generate_modules('minimal-git', gen, node_version)

    flatpak_builder.build(
        sources=itertools.chain(gen.ordered_sources()),
        commands=[
            provider_factory_spec.install_command,
            """node -e 'require("nop")'""",
        ],
        use_node=node_version,
    )


async def test_git(
    flatpak_builder: FlatpakBuilder,
    provider_factory_spec: ProviderFactorySpec,
    node_version: int,
    shared_datadir: Path,
) -> None:
    with ManifestGenerator() as gen:
        await provider_factory_spec.generate_modules('git', gen, node_version)

    flatpak_builder.build(
        sources=itertools.chain(
            gen.ordered_sources(),
            [
                {
                    'type': 'dir',
                    'path': str(shared_datadir / 'packages' / 'git' / 'subdir'),
                    'dest': 'subdir',
                }
            ],
        ),
        commands=[
            provider_factory_spec.install_command,
            f"""node -e '{TEST_SCRIPT}'""",
        ],
        use_node=node_version,
    )


async def test_local(
    flatpak_builder: FlatpakBuilder,
    provider_factory_spec: ProviderFactorySpec,
    node_version: int,
    shared_datadir: Path,
) -> None:
    with ManifestGenerator() as gen:
        await provider_factory_spec.generate_modules('local', gen, node_version)

    flatpak_builder.build(
        sources=itertools.chain(
            gen.ordered_sources(),
            [
                {
                    'type': 'dir',
                    'path': str(shared_datadir / 'packages' / 'local' / 'subdir'),
                    'dest': 'subdir',
                }
            ],
        ),
        commands=[
            provider_factory_spec.install_command,
            """node -e 'require("subdir").sayHello()'""",
        ],
        use_node=node_version,
    )

    hello_txt = flatpak_builder.module_dir / 'hello.txt'
    assert hello_txt.read_text() == 'Hello!'


async def test_local_link(
    flatpak_builder: FlatpakBuilder,
    yarn_provider_factory_spec: ProviderFactorySpec,
    node_version: int,
    shared_datadir: Path,
) -> None:
    with ManifestGenerator() as gen:
        await yarn_provider_factory_spec.generate_modules(
            'local-link-yarn', gen, node_version
        )

    flatpak_builder.build(
        sources=itertools.chain(
            gen.ordered_sources(),
            [
                {
                    'type': 'dir',
                    'path': str(
                        shared_datadir / 'packages' / 'local-link-yarn' / 'subdir'
                    ),
                    'dest': 'subdir',
                }
            ],
        ),
        commands=[
            yarn_provider_factory_spec.install_command,
            """node -e 'require("subdir").sayHello()'""",
        ],
        use_node=node_version,
    )

    hello_txt = flatpak_builder.module_dir / 'hello.txt'
    assert hello_txt.read_text() == 'Hello!'


async def test_missing_resolved_field(
    flatpak_builder: FlatpakBuilder,
    npm_provider_factory_spec: ProviderFactorySpec,
    node_version: int,
) -> None:
    if node_version < 16:
        pytest.skip()

    with ManifestGenerator() as gen:
        await npm_provider_factory_spec.generate_modules(
            'missing-resolved-npm', gen, node_version
        )

    flatpak_builder.build(
        sources=gen.ordered_sources(),
        commands=[
            npm_provider_factory_spec.install_command,
            """node -e 'require("word-wrap")'""",
        ],
        use_node=node_version,
    )

    word_wrap_package_json = (
        flatpak_builder.module_dir / 'node_modules' / 'word-wrap' / 'package.json'
    )
    assert word_wrap_package_json.exists()


async def test_url_as_dep(
    flatpak_builder: FlatpakBuilder,
    npm_provider_factory_spec: ProviderFactorySpec,
    node_version: int,
) -> None:
    with ManifestGenerator() as gen:
        await npm_provider_factory_spec.generate_modules(
            'url-as-dep', gen, node_version
        )

    flatpak_builder.build(
        sources=gen.ordered_sources(),
        commands=[
            npm_provider_factory_spec.install_command,
            """node -e 'require("word-wrap")'""",
        ],
        use_node=node_version,
    )

    word_wrap_package_json = (
        flatpak_builder.module_dir / 'node_modules' / 'word-wrap' / 'package.json'
    )
    assert word_wrap_package_json.exists()


async def test_special_electron(
    flatpak_builder: FlatpakBuilder,
    provider_factory_spec: ProviderFactorySpec,
    node_version: int,
) -> None:
    VERSION = '26.3.0'
    SCRIPT = f"""
    import {{download}} from '@electron/get'
    await download('{VERSION}')
    """

    with ManifestGenerator() as gen:
        await provider_factory_spec.generate_modules('electron', gen, node_version)

    flatpak_builder.build(
        sources=itertools.chain(gen.ordered_sources()),
        commands=[
            provider_factory_spec.install_command,
            f"""node --input-type=module -e {shlex.quote(SCRIPT)}""",
        ],
        use_node=node_version,
    )

    electron_version = (
        flatpak_builder.module_dir / 'node_modules' / 'electron' / 'dist' / 'version'
    )
    assert electron_version.read_text() == '26.3.0'


async def test_special_esbuild(
    flatpak_builder: FlatpakBuilder,
    npm_provider_factory_spec: ProviderFactorySpec,
    node_version: int,
) -> None:
    with ManifestGenerator() as gen:
        await npm_provider_factory_spec.generate_modules(
            'esbuild-test', gen, node_version
        )
    flatpak_builder.build(
        sources=itertools.chain(gen.ordered_sources()),
        commands=[
            npm_provider_factory_spec.install_command,
            'flatpak-node/cache/esbuild/bin/esbuild-current --version',
        ],
        use_node=node_version,
    )
