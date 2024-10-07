from __future__ import annotations

from pathlib import Path

import subprocess
import urllib.parse

from conftest import FlatpakBuilder, RequestsController
from flatpak_node_generator.integrity import Integrity
from flatpak_node_generator.manifest import ManifestGenerator


def test_url(requests: RequestsController, flatpak_builder: FlatpakBuilder) -> None:
    DATA_1 = 'abc'
    DATA_2 = 'def'

    requests.server.expect_oneshot_request('/file1', 'GET').respond_with_data(DATA_1)
    requests.server.expect_oneshot_request('/file2', 'GET').respond_with_data(DATA_2)

    with ManifestGenerator() as gen:
        gen.add_url_source(requests.url_for('file1'), Integrity.generate(DATA_1))
        gen.add_url_source(
            requests.url_for('file2'),
            Integrity.generate(DATA_2),
            destination=Path('file2-renamed'),
        )

    flatpak_builder.build(sources=gen.ordered_sources())

    assert (flatpak_builder.module_dir / 'file1').read_text() == DATA_1
    assert (flatpak_builder.module_dir / 'file2-renamed').read_text() == DATA_2


def test_data(flatpak_builder: FlatpakBuilder) -> None:
    DATA_STR = 'abc'
    DATA_BYTES = b'def\0ghi'

    with ManifestGenerator() as gen:
        gen.add_data_source(DATA_STR, Path('str-file'))
        gen.add_data_source(DATA_BYTES, Path('bytes-file'))

    flatpak_builder.build(sources=gen.ordered_sources())

    assert (flatpak_builder.module_dir / 'str-file').read_text() == DATA_STR
    assert (flatpak_builder.module_dir / 'bytes-file').read_bytes() == DATA_BYTES


def test_local_file(flatpak_builder: FlatpakBuilder, tmp_path: Path) -> None:
    DATA = 'abc'

    path = tmp_path / 'file'
    path.write_text(DATA)

    with ManifestGenerator() as gen:
        gen.add_local_file_source(path)

    flatpak_builder.build(sources=gen.ordered_sources())

    assert (flatpak_builder.module_dir / path.name).read_text() == DATA


def test_git(tmp_path: Path, flatpak_builder: FlatpakBuilder) -> None:
    DATA_1 = 'this is a file in a git repo'
    DATA_2 = 'this is a changed file'

    git_repo = tmp_path / 'git-repo'
    git_repo.mkdir()

    test_file = git_repo / 'test-file'
    test_file.write_text(DATA_1)

    subprocess.run(['git', 'init'], cwd=git_repo, check=True)
    subprocess.run(
        ['git', 'symbolic-ref', 'HEAD', 'refs/heads/main'], cwd=git_repo, check=True
    )
    subprocess.run(['git', 'add', test_file.name], cwd=git_repo, check=True)
    subprocess.run(['git', 'commit', '-m', 'initial'], cwd=git_repo, check=True)

    main_ref = git_repo / '.git' / 'refs' / 'heads' / 'main'
    commit_1 = main_ref.read_text().strip()

    test_file.write_text(DATA_2)
    subprocess.run(['git', 'commit', '-am', 'change'], cwd=git_repo, check=True)

    commit_2 = main_ref.read_text().strip()

    with ManifestGenerator() as gen:
        url = f'file://{urllib.parse.quote(str(git_repo.absolute()))}'
        gen.add_git_source(url, commit=commit_1)
        gen.add_git_source(url, commit=commit_2, destination=Path('subdir'))

    flatpak_builder.build(sources=gen.ordered_sources())

    assert (flatpak_builder.module_dir / test_file.name).read_text() == DATA_1
    assert (
        flatpak_builder.module_dir / 'subdir' / test_file.name
    ).read_text() == DATA_2


def test_script(flatpak_builder: FlatpakBuilder) -> None:
    COMMANDS = ['echo 123']

    with ManifestGenerator() as gen:
        gen.add_script_source(COMMANDS, Path('script'))

    flatpak_builder.build(sources=gen.ordered_sources())

    with (flatpak_builder.module_dir / 'script').open() as fp:
        assert fp.readline().startswith('#!')
        assert fp.readline().strip() == 'echo 123'


def test_commands(flatpak_builder: FlatpakBuilder) -> None:
    COMMAND = 'echo 123 > test'

    with ManifestGenerator() as gen:
        gen.add_command(COMMAND)

    flatpak_builder.build(sources=gen.ordered_sources())

    test_file = flatpak_builder.module_dir / 'test'
    assert test_file.read_text().strip() == '123'


def test_ordering() -> None:
    URL_1 = 'abc'
    URL_2 = 'def'
    URL_3 = 'ghi'

    with ManifestGenerator() as gen:
        gen.add_archive_source(URL_3, Integrity.generate(''))
        gen.add_archive_source(URL_1, Integrity.generate(''))
        gen.add_archive_source(URL_2, Integrity.generate(''))

    sources = gen.ordered_sources()

    assert next(sources)['url'] == URL_1
    assert next(sources)['url'] == URL_2
    assert next(sources)['url'] == URL_3
