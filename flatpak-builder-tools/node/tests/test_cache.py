import hashlib
from pathlib import Path

from pytest import MonkeyPatch

from flatpak_node_generator.cache import Cache, FilesystemBasedCache


def test_cache_uses_hashed_filename(tmp_path: Path) -> None:
    cache = FilesystemBasedCache(tmp_path)
    Cache.instance = cache

    key = 'remote-url-metadata:size:https://example.com/very/long/url'
    bucket = cache.get(key)

    with bucket.open_write() as writer:
        writer.write(b'123')

    files = list(tmp_path.iterdir())
    assert len(files) == 1

    filename = files[0].name

    assert len(filename) == 64
    assert filename == hashlib.sha256(key.encode('utf-8')).hexdigest()


def test_cache_migrates_legacy_file(tmp_path: Path) -> None:
    cache = FilesystemBasedCache(tmp_path)
    Cache.instance = cache

    key = 'remote-url-metadata:size:https://example.com/legacy'
    legacy_name = FilesystemBasedCache._escape_key(key)
    legacy_path = tmp_path / legacy_name

    legacy_path.write_bytes(b'legacy-data')

    bucket = cache.get(key)

    reader = bucket.open_read()
    assert reader is not None
    assert reader.read_all() == b'legacy-data'
    reader.close()

    files = list(tmp_path.iterdir())
    assert len(files) == 1

    expected_hash = hashlib.sha256(key.encode('utf-8')).hexdigest()
    assert files[0].name == expected_hash


def test_cache_fallback_if_migration_fails(
    tmp_path: Path, monkeypatch: MonkeyPatch
) -> None:
    cache = FilesystemBasedCache(tmp_path)
    Cache.instance = cache

    key = 'remote-url-metadata:size:https://example.com/fallback'
    legacy_name = FilesystemBasedCache._escape_key(key)
    legacy_path = tmp_path / legacy_name

    legacy_path.write_bytes(b'fallback-data')

    def fail_rename(self: Path, target: Path) -> None:
        raise OSError('rename failed')

    monkeypatch.setattr(Path, 'rename', fail_rename)

    bucket = cache.get(key)

    reader = bucket.open_read()
    assert reader is not None
    assert reader.read_all() == b'fallback-data'
    reader.close()

    assert legacy_path.exists()

    expected_hash = hashlib.sha256(key.encode('utf-8')).hexdigest()
    assert not (tmp_path / expected_hash).exists()


def test_cache_never_creates_escaped_filename(tmp_path: Path) -> None:
    cache = FilesystemBasedCache(tmp_path)
    Cache.instance = cache

    key = 'remote-url-metadata:size:https://example.com/test'
    escaped_name = FilesystemBasedCache._escape_key(key)

    bucket = cache.get(key)

    with bucket.open_write() as writer:
        writer.write(b'data')

    assert not (tmp_path / escaped_name).exists()

    expected_hash = hashlib.sha256(key.encode('utf-8')).hexdigest()
    assert (tmp_path / expected_hash).exists()
