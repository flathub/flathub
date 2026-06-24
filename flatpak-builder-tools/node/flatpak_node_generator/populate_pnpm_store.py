from __future__ import annotations

import base64
import contextlib
import hashlib
import json
import os
import re
import sqlite3
import struct
import sys
import tarfile
import time
from collections.abc import Mapping

_SANITIZE_RE = re.compile(r'[\\/:*?"<>|]')
_MAX_LENGTH_WITHOUT_HASH = 120


def _msgpack_pack(obj: object) -> bytes:
    """Minimal msgpack packer matching msgpackr with useRecords: true.

    msgpackr reserves the byte range 0x40-0x7F for record IDs when useRecords
    is enabled (pnpm's config).  Standard msgpack uses 0x00-0x7F for positive
    fixints, but this packer caps them at 0x3F and emits a uint 8 prefix (0xcc)
    for 64-255 to avoid aliasing with record ID bytes.
    """

    def _pack_int(val: int) -> bytes:
        # Positive fixint capped at 0x3F — 0x40-0x7F are msgpackr record IDs
        if 0 <= val <= 0x3F:
            return val.to_bytes(1, 'big')
        if -32 <= val < 0:
            return val.to_bytes(1, 'big', signed=True)
        if val >= 0:
            if val <= 0xFF:
                return b'\xcc' + val.to_bytes(1, 'big')
            if val <= 0xFFFF:
                return b'\xcd' + val.to_bytes(2, 'big')
            if val <= 0xFFFFFFFF:
                return b'\xce' + val.to_bytes(4, 'big')
            return b'\xcf' + val.to_bytes(8, 'big')
        if val >= -0x80:
            return b'\xd0' + val.to_bytes(1, 'big', signed=True)
        if val >= -0x8000:
            return b'\xd1' + val.to_bytes(2, 'big', signed=True)
        if val >= -0x80000000:
            return b'\xd2' + val.to_bytes(4, 'big', signed=True)
        return b'\xd3' + val.to_bytes(8, 'big', signed=True)

    def _pack_str(val: str) -> bytes:
        data = val.encode('utf-8')
        length = len(data)
        if length <= 0x1F:
            return (0xA0 | length).to_bytes(1, 'big') + data
        if length <= 0xFF:
            return b'\xd9' + length.to_bytes(1, 'big') + data
        if length <= 0xFFFF:
            return b'\xda' + length.to_bytes(2, 'big') + data
        return b'\xdb' + length.to_bytes(4, 'big') + data

    if obj is None:
        return b'\xc0'
    if isinstance(obj, bool):
        return b'\xc3' if obj else b'\xc2'
    if isinstance(obj, int):
        return _pack_int(obj)
    if isinstance(obj, float):
        return b'\xcb' + struct.pack('>d', obj)
    if isinstance(obj, str):
        return _pack_str(obj)
    if isinstance(obj, dict):
        length = len(obj)
        if length <= 0x0F:
            result = (0x80 | length).to_bytes(1, 'big')
        elif length <= 0xFFFF:
            result = b'\xde' + length.to_bytes(2, 'big')
        else:
            result = b'\xdf' + length.to_bytes(4, 'big')
        for key, val in obj.items():
            result += _msgpack_pack(key) + _msgpack_pack(val)
        return result
    if isinstance(obj, (list, tuple)):
        length = len(obj)
        if length <= 0x0F:
            result = (0x90 | length).to_bytes(1, 'big')
        elif length <= 0xFFFF:
            result = b'\xdc' + length.to_bytes(2, 'big')
        else:
            result = b'\xdd' + length.to_bytes(4, 'big')
        for item in obj:
            result += _msgpack_pack(item)
        return result
    raise TypeError(f'Unsupported type for msgpack: {type(obj)}')


RECORD_HEADER = b'\xd4\x72'


def _pack_v11_store_entry(
    files: dict[str, dict[str, object]],
    manifest: dict[str, str] | None = None,
) -> bytes:
    """Encode a store v11 entry using msgpackr-compatible record extensions.

    msgpackr with ``useRecords: true, moreTypes: true`` decodes:

    - standard msgpack maps (0x80/0xde/0xdf) as JavaScript ``Map`` objects
      (iterable, supports ``for..of``)
    - record extensions (0x72) as plain objects (dot-notation access)

    The ``files`` field must be a Map so pnpm can iterate it with ``for..of``.
    Everything else must use record extensions so dot-notation works
    (e.g. ``pkgIndex.algo``, ``info.digest``, ``manifest.name``).
    """

    def _record(obj: Mapping[str, object], struct_id: int) -> bytes:
        keys = list(obj.keys())
        result = RECORD_HEADER + struct_id.to_bytes(1, 'big')
        result += _msgpack_pack(keys)
        for k in keys:
            result += _msgpack_pack(obj[k])
        return result

    def _fixmap(length: int) -> bytes:
        if length <= 0x0F:
            return (0x80 | length).to_bytes(1, 'big')
        if length <= 0xFFFF:
            return b'\xde' + length.to_bytes(2, 'big')
        return b'\xdf' + length.to_bytes(4, 'big')

    # Pack file entries as records, build files map as standard msgpack map
    files_map_bytes = _fixmap(len(files))
    for fname, finfo in files.items():
        files_map_bytes += _msgpack_pack(fname) + _record(finfo, 0x41)

    # Outer store_entry as record (struct_id 0x40)
    store_entry_keys = ['algo', 'requiresBuild', 'files']
    if manifest is not None:
        store_entry_keys.append('manifest')

    # Record with fixed entries
    result = RECORD_HEADER + b'\x40' + _msgpack_pack(store_entry_keys)
    # This is hardcoded to use sha512 per pnpm's behavior, see @pnpm/store.cafs/index and
    # file digest logic below
    result += _msgpack_pack('sha512')  # algo
    result += _msgpack_pack(False)  # requiresBuild
    result += files_map_bytes  # files (standard map → iterable Map in JS)

    if manifest is not None:
        result += _record(manifest, 0x42)
    return result


def populate_store(manifest_path: str, tarball_dir: str, store_dir: str) -> None:
    with open(manifest_path, encoding='utf-8') as f:
        manifest = json.load(f)

    store_version = manifest['store_version']
    packages = manifest['packages']

    index_db: sqlite3.Connection | None = None

    store = os.path.join(store_dir, store_version)
    os.makedirs(os.path.join(store, 'files'), exist_ok=True)
    if store_version == 'v11':
        index_db = sqlite3.connect(os.path.join(store, 'index.db'))
        index_db.execute('PRAGMA busy_timeout=5000')
        index_db.execute('PRAGMA journal_mode=WAL')
        index_db.execute('PRAGMA synchronous=NORMAL')
        index_db.execute('PRAGMA temp_store=MEMORY')
        index_db.execute(
            'CREATE TABLE IF NOT EXISTS package_index ('
            '  key TEXT PRIMARY KEY,'
            '  data BLOB NOT NULL'
            ') WITHOUT ROWID'
        )
    else:
        os.makedirs(os.path.join(store, 'index'), exist_ok=True)

    now = int(time.time() * 1000)

    for tarball_name, info in packages.items():
        tarball_path = os.path.join(tarball_dir, tarball_name)
        if not os.path.isfile(tarball_path):
            raise FileNotFoundError(tarball_path)

        _process_tarball(
            tarball_path=tarball_path,
            pkg_name=info['name'],
            pkg_version=info['version'],
            integrity=info['integrity'],
            integrity_digest=info['integrity_digest'],
            integrity_algo=info['integrity_algo'],
            store=store,
            now=now,
            tarball_url=info.get('tarball_url'),
            store_version=store_version,
            index_db=index_db,
        )

    if index_db is not None:
        index_db.commit()
        index_db.close()


def _process_tarball(
    *,
    tarball_path: str,
    pkg_name: str,
    pkg_version: str,
    integrity: str,
    integrity_digest: str,
    integrity_algo: str,
    store: str,
    now: int,
    tarball_url: str | None = None,
    store_version: str = 'v3',
    index_db: sqlite3.Connection | None = None,
) -> None:
    index_files: dict[str, dict[str, object]] = {}
    file_digests: dict[str, str] = {}
    real_pkg_name = pkg_name
    real_pkg_version = pkg_version

    with tarfile.open(tarball_path, 'r:gz') as tf:
        for member in tf.getmembers():
            if not member.isfile():
                continue
            fobj = tf.extractfile(member)
            if fobj is None:
                continue
            data = fobj.read()

            if member.name.endswith('package.json') and member.name.count('/') <= 1:
                with contextlib.suppress(ValueError, TypeError, UnicodeDecodeError):
                    pkg_data = json.loads(data.decode('utf-8'))
                    if isinstance(pkg_data, dict):
                        if 'name' in pkg_data and isinstance(pkg_data['name'], str):
                            real_pkg_name = pkg_data['name']
                        if 'version' in pkg_data and isinstance(
                            pkg_data['version'], str
                        ):
                            real_pkg_version = pkg_data['version']

            digest = hashlib.sha512(data).digest()
            file_hex = digest.hex()
            is_exec = bool(member.mode & 0o111)

            cas_dir = os.path.join(store, 'files', file_hex[:2])
            cas_name = file_hex[2:] + ('-exec' if is_exec else '')
            cas_path = os.path.join(cas_dir, cas_name)
            if not os.path.exists(cas_path):
                os.makedirs(cas_dir, exist_ok=True)
                with open(cas_path, 'wb') as out:
                    out.write(data)
                if is_exec:
                    os.chmod(cas_path, 0o755)

            rel_name = member.name
            if '/' in rel_name:
                rel_name = rel_name.split('/', 1)[1]

            b64 = base64.b64encode(digest).decode()
            index_files[rel_name] = {
                'checkedAt': now,
                'integrity': f'sha512-{b64}',
                'mode': member.mode,
                'size': len(data),
            }
            file_digests[rel_name] = file_hex

    if store_version == 'v11':
        assert index_db is not None
        # pnpm v11 store index keys use the raw package name (e.g. @scope/pkg),
        # not the filesystem-sanitized form (@scope+pkg), since keys are stored
        # in SQLite, not as filenames.
        raw_pkg_id = f'{pkg_name}@{pkg_version}'
        key = f'{integrity_algo}-{integrity}\t{raw_pkg_id}'

        v11_files: dict[str, dict[str, object]] = {}
        for rel_name, finfo in index_files.items():
            checked_at = finfo['checkedAt']
            assert isinstance(checked_at, int)
            v11_files[rel_name] = {
                'checkedAt': float(checked_at),  # float64 avoids msgpackr BigInt
                'digest': file_digests[rel_name],
                'mode': finfo['mode'],
                'size': finfo['size'],
            }

        manifest = None
        if real_pkg_name or real_pkg_version:
            manifest = {
                'name': real_pkg_name,
                'version': real_pkg_version,
            }

        entry_bytes = _pack_v11_store_entry(v11_files, manifest)

        # It's currently not possible to fully determine which store key pnpm will use,
        # so we insert multiple keys to ensure pnpm can find the entry it wants.

        index_db.execute(
            'INSERT OR REPLACE INTO package_index (key, data) VALUES (?, ?)',
            (key, entry_bytes),
        )

        if tarball_url:
            url_key = f'{tarball_url}\t{raw_pkg_id}'
            index_db.execute(
                'INSERT OR REPLACE INTO package_index (key, data) VALUES (?, ?)',
                (url_key, entry_bytes),
            )

            # pnpm looks up git-hosted tarballs (codeload.github.com,
            # bitbucket.org, gitlab.com) and tarballs without integrity by
            # {tarball_url}\tbuilt(not-built) — see pickStoreIndexKey in @pnpm/store.index.
            pkgid_key = f'{tarball_url}\t'
            index_db.execute(
                'INSERT OR REPLACE INTO package_index (key, data) VALUES (?, ?)',
                (pkgid_key + 'built', entry_bytes),
            )
            index_db.execute(
                'INSERT OR REPLACE INTO package_index (key, data) VALUES (?, ?)',
                (pkgid_key + 'not-built', entry_bytes),
            )
    else:
        pkg_id = _SANITIZE_RE.sub('+', f'{pkg_name}@{pkg_version}')

        index_data = {
            'name': real_pkg_name,
            'version': real_pkg_version,
            'requiresBuild': False,
            'files': index_files,
        }

        idx_prefix = integrity_digest[:2]
        idx_rest = integrity_digest[2:64]
        idx_dir = os.path.join(store, 'index', idx_prefix)
        os.makedirs(idx_dir, exist_ok=True)
        idx_path = os.path.join(idx_dir, f'{idx_rest}-{pkg_id}.json')
        with open(idx_path, 'w', encoding='utf-8') as out:
            json.dump(index_data, out)

        # For tarball-URL packages, also create an index entry keyed by the URL hash
        # this is how pnpm looks up tarball deps without integrity
        if tarball_url:
            if store_version == 'v3':
                url_hash = hashlib.sha256(tarball_url.encode()).hexdigest()
                url_idx_prefix = url_hash[:2]
                url_idx_rest = url_hash[2:64]
                url_idx_dir = os.path.join(store, 'index', url_idx_prefix)
                os.makedirs(url_idx_dir, exist_ok=True)
                url_idx_path = os.path.join(
                    url_idx_dir, f'{url_idx_rest}-{pkg_id}.json'
                )
                with open(url_idx_path, 'w', encoding='utf-8') as out:
                    json.dump(index_data, out)
            else:
                url_dir_name = re.sub(r'[:/]', '+', tarball_url)
                if (
                    len(url_dir_name) > _MAX_LENGTH_WITHOUT_HASH
                    or url_dir_name != url_dir_name.lower()
                ):
                    url_dir_name = f'{url_dir_name[: _MAX_LENGTH_WITHOUT_HASH - 33]}_{hashlib.sha256(url_dir_name.encode()).hexdigest()[:32]}'
                url_idx_dir = os.path.join(store, url_dir_name)
                os.makedirs(url_idx_dir, exist_ok=True)
                url_idx_path = os.path.join(url_idx_dir, 'integrity.json')
                with open(url_idx_path, 'w', encoding='utf-8') as out:
                    json.dump(index_data, out)


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(
            f'Usage: {sys.argv[0]} <manifest.json> <tarball-dir> <store-dir>',
            file=sys.stderr,
        )
        sys.exit(1)
    populate_store(sys.argv[1], sys.argv[2], sys.argv[3])
