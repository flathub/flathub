import hashlib
import json
import re
import sqlite3
import struct
import tarfile
from pathlib import Path

from flatpak_node_generator.integrity import Integrity
from flatpak_node_generator.populate_pnpm_store import (
    _pack_v11_store_entry,
    _process_tarball,
    populate_store,
)


def _decode_v11(data: bytes) -> dict[str, object]:
    """Decode msgpackr record-extension encoding to plain dicts/lists.

    msgpackr with ``useRecords: true, moreTypes: true`` encodes:

    - plain objects as |d4 72 struct_id fixarray-of-keys values...|
    - Maps as standard msgpack maps (0x80/0xde/0xdf)

    This decoder returns nested dicts (for both records and maps) so the
    test assertions can use dictionary-style access.
    """

    def _read(offset: int) -> tuple[object, int]:
        if offset >= len(data):
            raise ValueError('unexpected end of data')
        byte = data[offset]
        offset += 1

        # positive fixint
        if byte <= 0x7F:
            return byte, offset
        # fixmap
        if 0x80 <= byte <= 0x8F:
            length = byte & 0x0F
            result: dict[str, object] = {}
            for _ in range(length):
                k, offset = _read_str(offset)
                v, offset = _read(offset)
                result[k] = v
            return result, offset
        # fixarray
        if 0x90 <= byte <= 0x9F:
            length = byte & 0x0F
            arr: list[object] = []
            for _ in range(length):
                v, offset = _read(offset)
                arr.append(v)
            return arr, offset
        # fixstr
        if 0xA0 <= byte <= 0xBF:
            length = byte & 0x1F
            return data[offset : offset + length].decode('utf-8'), offset + length
        # negative fixint
        if 0xE0 <= byte <= 0xFF:
            return byte - 256, offset
        # nil
        if byte == 0xC0:
            return None, offset
        # false / true
        if byte == 0xC2:
            return False, offset
        if byte == 0xC3:
            return True, offset
        # bin 8/16/32
        if byte == 0xC4:
            return data[offset : offset + data[offset]], offset + 1 + data[offset]
        if byte == 0xC5:
            length = int.from_bytes(data[offset : offset + 2], 'big')
            return data[offset + 2 : offset + 2 + length], offset + 2 + length
        if byte == 0xC6:
            length = int.from_bytes(data[offset : offset + 4], 'big')
            return data[offset + 4 : offset + 4 + length], offset + 4 + length
        # float 32
        if byte == 0xCA:
            return struct.unpack('>f', data[offset : offset + 4])[0], offset + 4
        # float 64
        if byte == 0xCB:
            return struct.unpack('>d', data[offset : offset + 8])[0], offset + 8
        # uint 8/16/32/64
        if byte == 0xCC:
            return data[offset], offset + 1
        if byte == 0xCD:
            return int.from_bytes(data[offset : offset + 2], 'big'), offset + 2
        if byte == 0xCE:
            return int.from_bytes(data[offset : offset + 4], 'big'), offset + 4
        if byte == 0xCF:
            return int.from_bytes(data[offset : offset + 8], 'big'), offset + 8
        # int 8/16/32/64
        if byte == 0xD0:
            return int.from_bytes(
                data[offset : offset + 1], 'big', signed=True
            ), offset + 1
        if byte == 0xD1:
            return int.from_bytes(
                data[offset : offset + 2], 'big', signed=True
            ), offset + 2
        if byte == 0xD2:
            return int.from_bytes(
                data[offset : offset + 4], 'big', signed=True
            ), offset + 4
        if byte == 0xD3:
            return int.from_bytes(
                data[offset : offset + 8], 'big', signed=True
            ), offset + 8
        # fixext 4 with record extension
        if byte == 0xD4 and data[offset] == 0x72:
            offset += 1  # skip ext type 0x72
            offset += 1  # skip struct_id byte
            keys, offset = _read(offset)
            assert isinstance(keys, list), f'expected array of keys, got {type(keys)}'
            record: dict[str, object] = {}
            for k in keys:
                v, offset = _read(offset)
                record[str(k)] = v
            return record, offset
        # str 8/16/32
        if byte == 0xD9:
            length = data[offset]
            return data[offset + 1 : offset + 1 + length].decode(
                'utf-8'
            ), offset + 1 + length
        if byte == 0xDA:
            length = int.from_bytes(data[offset : offset + 2], 'big')
            return data[offset + 2 : offset + 2 + length].decode(
                'utf-8'
            ), offset + 2 + length
        if byte == 0xDB:
            length = int.from_bytes(data[offset : offset + 4], 'big')
            return data[offset + 4 : offset + 4 + length].decode(
                'utf-8'
            ), offset + 4 + length
        # map 16/32
        if byte == 0xDE:
            length = int.from_bytes(data[offset : offset + 2], 'big')
            offset += 2
            m: dict[str, object] = {}
            for _ in range(length):
                k, offset = _read_str(offset)
                v, offset = _read(offset)
                m[k] = v
            return m, offset
        if byte == 0xDF:
            length = int.from_bytes(data[offset : offset + 4], 'big')
            offset += 4
            m2: dict[str, object] = {}
            for _ in range(length):
                k, offset = _read_str(offset)
                v, offset = _read(offset)
                m2[k] = v
            return m2, offset
        # array 16/32
        if byte == 0xDC:
            length = int.from_bytes(data[offset : offset + 2], 'big')
            offset += 2
            arr2: list[object] = []
            for _ in range(length):
                v, offset = _read(offset)
                arr2.append(v)
            return arr2, offset
        if byte == 0xDD:
            length = int.from_bytes(data[offset : offset + 4], 'big')
            offset += 4
            arr3: list[object] = []
            for _ in range(length):
                v, offset = _read(offset)
                arr3.append(v)
            return arr3, offset

        raise ValueError(
            f'unsupported msgpack byte: 0x{byte:02X} at offset {offset - 1}'
        )

    def _read_str(offset: int) -> tuple[str, int]:
        val, offset = _read(offset)
        assert isinstance(val, str), f'expected str, got {type(val)}'
        return val, offset

    result, offset = _read(0)
    if offset != len(data):
        raise ValueError(f'extra bytes at end: {offset} < {len(data)}')
    assert isinstance(result, dict)
    return result


def _create_tarball(path: Path, files: dict[str, str | bytes]) -> None:
    with tarfile.open(path, 'w:gz') as tf:
        for name, data in files.items():
            content = data if isinstance(data, bytes) else data.encode('utf-8')
            tmp_file = path.parent / 'tmp_member'
            tmp_file.write_bytes(content)

            tarinfo = tarfile.TarInfo(name)
            tarinfo.size = len(content)
            tarinfo.mode = 0o644

            with open(tmp_file, 'rb') as f:
                tf.addfile(tarinfo, f)
            tmp_file.unlink()


def test_process_tarball_normal(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store'
    pkg_json = json.dumps({'name': 'real-pkg', 'version': '1.2.3'})

    _create_tarball(
        tar_path,
        {'package/package.json': pkg_json, 'package/index.js': "console.log('hello');"},
    )

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    _process_tarball(
        tarball_path=str(tar_path),
        pkg_name='fallback-pkg',
        pkg_version='0.0.0',
        integrity=integrity.to_base64(),
        integrity_digest=integrity.digest,
        integrity_algo=integrity.algorithm,
        store=str(store_dir),
        now=1234567890,
    )

    idx_files = list((store_dir / 'index' / 'a1').glob('*.json'))
    assert len(idx_files) == 1

    with open(idx_files[0], 'r', encoding='utf-8') as f:
        data = json.load(f)

    assert data['name'] == 'real-pkg'
    assert data['version'] == '1.2.3'
    assert data['requiresBuild'] is False
    assert 'package.json' in data['files']
    assert 'index.js' in data['files']


def test_process_tarball_malformed_package_json(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store'

    _create_tarball(tar_path, {'package/package.json': '{ malformed: json '})

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    _process_tarball(
        tarball_path=str(tar_path),
        pkg_name='fallback-pkg',
        pkg_version='0.0.0',
        integrity=integrity.to_base64(),
        integrity_digest=integrity.digest,
        integrity_algo=integrity.algorithm,
        store=str(store_dir),
        now=1234567890,
    )

    idx_files = list((store_dir / 'index' / 'a1').glob('*.json'))
    assert len(idx_files) == 1

    with open(idx_files[0], 'r', encoding='utf-8') as f:
        data = json.load(f)

    assert data['name'] == 'fallback-pkg'
    assert data['version'] == '0.0.0'


def test_process_tarball_with_tarball_url_v3(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store'
    tarball_url = 'https://example.com/pkg.tgz'

    _create_tarball(tar_path, {'package/index.js': "console.log('hello');"})

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    _process_tarball(
        tarball_path=str(tar_path),
        pkg_name='pkg',
        pkg_version='1.0.0',
        integrity=integrity.to_base64(),
        integrity_digest=integrity.digest,
        integrity_algo=integrity.algorithm,
        store=str(store_dir),
        now=1234567890,
        tarball_url=tarball_url,
        store_version='v3',
    )

    import hashlib

    url_hash = hashlib.sha256(tarball_url.encode()).hexdigest()
    url_idx_dir = store_dir / 'index' / url_hash[:2]

    assert url_idx_dir.exists()
    assert len(list(url_idx_dir.glob('*.json'))) == 1


def test_process_tarball_with_tarball_url_v6(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store'
    tarball_url = 'https://example.com/pkg.tgz'

    _create_tarball(tar_path, {'package/index.js': "console.log('hello');"})

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    _process_tarball(
        tarball_path=str(tar_path),
        pkg_name='pkg',
        pkg_version='1.0.0',
        integrity=integrity.to_base64(),
        integrity_digest=integrity.digest,
        integrity_algo=integrity.algorithm,
        store=str(store_dir),
        now=1234567890,
        tarball_url=tarball_url,
        store_version='v6',
    )

    url_dir_name = re.sub(r'[:/]', '+', tarball_url)
    url_idx_file = store_dir / url_dir_name / 'integrity.json'

    assert url_idx_file.exists()


def test_process_tarball_with_uppercase_path(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store'
    tarball_url = 'https://example.com/PKG.tgz'

    _create_tarball(tar_path, {'package/index.js': "console.log('hello');"})

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    _process_tarball(
        tarball_path=str(tar_path),
        pkg_name='pkg',
        pkg_version='1.0.0',
        integrity=integrity.to_base64(),
        integrity_digest=integrity.digest,
        integrity_algo=integrity.algorithm,
        store=str(store_dir),
        now=1234567890,
        tarball_url=tarball_url,
        store_version='v6',
    )

    sanitized_tarball_url = re.sub(r'[:/]', '+', tarball_url)
    normalized_tarball_url = f'{sanitized_tarball_url}_{hashlib.sha256(sanitized_tarball_url.encode()).hexdigest()[:32]}'
    url_idx_file = store_dir / normalized_tarball_url / 'integrity.json'

    assert url_idx_file.exists()


def test_process_tarball_with_long_path(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store'
    tarball_url = f'https://example.com{"pkg" * 50}.tgz'

    _create_tarball(tar_path, {'package/index.js': "console.log('hello');"})

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    _process_tarball(
        tarball_path=str(tar_path),
        pkg_name='pkg',
        pkg_version='1.0.0',
        integrity=integrity.to_base64(),
        integrity_digest=integrity.digest,
        integrity_algo=integrity.algorithm,
        store=str(store_dir),
        now=1234567890,
        tarball_url=tarball_url,
        store_version='v6',
    )

    sanitized_tarball_url = re.sub(r'[:/]', '+', tarball_url)
    normalized_tarball_url = f'{sanitized_tarball_url[:87]}_{hashlib.sha256(sanitized_tarball_url.encode()).hexdigest()[:32]}'
    url_idx_file = store_dir / normalized_tarball_url / 'integrity.json'

    assert url_idx_file.exists()


def test_populate_store_v11(tmp_path: Path) -> None:
    manifest_path = tmp_path / 'manifest.json'
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store'
    pkg_json = json.dumps({'name': 'real-pkg', 'version': '1.2.3'})

    integrity = Integrity(
        'sha512', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    _create_tarball(
        tar_path,
        {'package/package.json': pkg_json, 'package/index.js': "console.log('hello');"},
    )

    manifest_json = json.dumps(
        {
            'store_version': 'v11',
            'packages': {
                'pkg.tgz': {
                    'name': 'real-pkg',
                    'version': '1.2.3',
                    'integrity': integrity.to_base64(),
                    'integrity_digest': integrity.digest,
                    'integrity_algo': integrity.algorithm,
                }
            },
        }
    )

    with open(manifest_path, 'w', encoding='utf-8') as f:
        f.write(manifest_json)

    populate_store(str(manifest_path), str(tmp_path), str(store_dir))

    db = sqlite3.connect(str(store_dir / 'v11' / 'index.db'))

    pkg_id = 'real-pkg@1.2.3'
    expected_key = f'{integrity.algorithm}-{integrity.to_base64()}\t{pkg_id}'

    row = db.execute(
        'SELECT data FROM package_index WHERE key = ?', (expected_key,)
    ).fetchone()
    assert row is not None, f'No row found for key {expected_key}'

    data = _decode_v11(row[0])
    assert data['algo'] == 'sha512'
    assert data['requiresBuild'] is False
    assert data['manifest'] == {'name': 'real-pkg', 'version': '1.2.3'}
    assert isinstance(data['files'], dict)
    assert 'package.json' in data['files']
    assert 'index.js' in data['files']


def test_process_tarball_v11(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store' / 'v11'
    pkg_json = json.dumps({'name': 'real-pkg', 'version': '1.2.3'})

    _create_tarball(
        tar_path,
        {'package/package.json': pkg_json, 'package/index.js': "console.log('hello');"},
    )

    store_dir.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(str(store_dir / 'index.db'))
    db.execute('PRAGMA busy_timeout=5000')
    db.execute('PRAGMA journal_mode=WAL')
    db.execute('PRAGMA synchronous=NORMAL')
    db.execute('PRAGMA temp_store=MEMORY')
    db.execute(
        'CREATE TABLE IF NOT EXISTS package_index ('
        '  key TEXT PRIMARY KEY,'
        '  data BLOB NOT NULL'
        ') WITHOUT ROWID'
    )

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    try:
        _process_tarball(
            tarball_path=str(tar_path),
            pkg_name='fallback-pkg',
            pkg_version='0.0.0',
            integrity=integrity.to_base64(),
            integrity_digest=integrity.digest,
            integrity_algo=integrity.algorithm,
            store=str(store_dir),
            now=1234567890,
            store_version='v11',
            index_db=db,
        )
        db.commit()

        pkg_id = 'fallback-pkg@0.0.0'
        expected_key = f'{integrity.algorithm}-{integrity.to_base64()}\t{pkg_id}'

        row = db.execute(
            'SELECT data FROM package_index WHERE key = ?', (expected_key,)
        ).fetchone()
        assert row is not None, f'No row found for key {expected_key}'

        data = _decode_v11(row[0])
        assert data['algo'] == 'sha512'
        assert data['requiresBuild'] is False
        assert data['manifest'] == {'name': 'real-pkg', 'version': '1.2.3'}
        assert isinstance(data['files'], dict)
        assert 'package.json' in data['files']
        assert 'index.js' in data['files']
    finally:
        db.close()


def test_process_tarball_v11_no_package_json(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store' / 'v11'

    _create_tarball(tar_path, {'package/index.js': "console.log('hello');"})

    store_dir.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(str(store_dir / 'index.db'))
    db.execute('PRAGMA busy_timeout=5000')
    db.execute(
        'CREATE TABLE IF NOT EXISTS package_index ('
        '  key TEXT PRIMARY KEY,'
        '  data BLOB NOT NULL'
        ') WITHOUT ROWID'
    )

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    try:
        _process_tarball(
            tarball_path=str(tar_path),
            pkg_name='no-manifest-pkg',
            pkg_version='2.0.0',
            integrity=integrity.to_base64(),
            integrity_digest=integrity.digest,
            integrity_algo=integrity.algorithm,
            store=str(store_dir),
            now=1234567890,
            store_version='v11',
            index_db=db,
        )
        db.commit()

        pkg_id = 'no-manifest-pkg@2.0.0'
        expected_key = f'{integrity.algorithm}-{integrity.to_base64()}\t{pkg_id}'

        row = db.execute(
            'SELECT data FROM package_index WHERE key = ?', (expected_key,)
        ).fetchone()
        assert row is not None

        data = _decode_v11(row[0])
        # Fallback name/version used since there's no real package.json
        assert data['manifest'] == {
            'name': 'no-manifest-pkg',
            'version': '2.0.0',
        }
    finally:
        db.close()


def test_process_tarball_v11_with_tarball_url(tmp_path: Path) -> None:
    tar_path = tmp_path / 'pkg.tgz'
    store_dir = tmp_path / 'store' / 'v11'
    tarball_url = 'https://example.com/pkg.tgz'

    _create_tarball(tar_path, {'package/index.js': "console.log('hello');"})

    store_dir.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(str(store_dir / 'index.db'))
    db.execute('PRAGMA busy_timeout=5000')
    db.execute(
        'CREATE TABLE IF NOT EXISTS package_index ('
        '  key TEXT PRIMARY KEY,'
        '  data BLOB NOT NULL'
        ') WITHOUT ROWID'
    )

    integrity = Integrity(
        'sha256', 'a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2'
    )

    try:
        _process_tarball(
            tarball_path=str(tar_path),
            pkg_name='pkg',
            pkg_version='1.0.0',
            integrity=integrity.to_base64(),
            integrity_digest=integrity.digest,
            integrity_algo=integrity.algorithm,
            store=str(store_dir),
            now=1234567890,
            store_version='v11',
            index_db=db,
            tarball_url=tarball_url,
        )
        db.commit()

        # Check main integrity-based entry exists
        pkg_id = 'pkg@1.0.0'
        main_key = f'{integrity.algorithm}-{integrity.to_base64()}\t{pkg_id}'
        main_row = db.execute(
            'SELECT data FROM package_index WHERE key = ?', (main_key,)
        ).fetchone()
        assert main_row is not None

        # Check tarball_url-based entry exists
        url_key = f'{tarball_url}\t{pkg_id}'
        url_row = db.execute(
            'SELECT data FROM package_index WHERE key = ?', (url_key,)
        ).fetchone()
        assert url_row is not None

        # Check pkgId-based entry exists ({pkgId}\tbuilt — git-hosted tarball format)
        pkgid_key = f'{tarball_url}\tbuilt'
        pkgid_row = db.execute(
            'SELECT data FROM package_index WHERE key = ?', (pkgid_key,)
        ).fetchone()
        assert pkgid_row is not None
    finally:
        db.close()


def test_pack_v11_store_entry() -> None:
    """Verify _pack_v11_store_entry produces msgpackr-compatible record encoding."""
    files = {
        'index.js': {
            'checkedAt': 1234567890,
            'digest': 'abc123',
            'mode': 420,
            'size': 100,
        },
        'package.json': {
            'checkedAt': 111111,
            'digest': 'def456',
            'mode': 384,
            'size': 200,
        },
    }
    manifest = {'name': 'test-pkg', 'version': '1.0.0'}
    packed = _pack_v11_store_entry(files, manifest)

    data = _decode_v11(packed)
    assert isinstance(data, dict)
    assert data['algo'] == 'sha512'
    assert data['requiresBuild'] is False
    assert isinstance(data['files'], dict)
    assert data['files']['index.js']['digest'] == 'abc123'
    assert data['files']['index.js']['mode'] == 420
    assert data['files']['index.js']['size'] == 100
    assert data['files']['index.js']['checkedAt'] == 1234567890
    assert data['files']['package.json']['digest'] == 'def456'
    assert data['manifest'] == manifest


def test_pack_v11_store_entry_no_manifest() -> None:
    """Verify _pack_v11_store_entry works without a manifest."""
    files = {
        'index.js': {'checkedAt': 123, 'digest': 'hex123', 'mode': 420, 'size': 50}
    }
    packed = _pack_v11_store_entry(files, None)

    data = _decode_v11(packed)
    assert isinstance(data, dict)
    assert data['algo'] == 'sha512'
    assert data['requiresBuild'] is False
    assert 'manifest' not in data
    files_obj = data['files']
    assert isinstance(files_obj, dict)
    index_js = files_obj['index.js']
    assert isinstance(index_js, dict)
    assert index_js['digest'] == 'hex123'
