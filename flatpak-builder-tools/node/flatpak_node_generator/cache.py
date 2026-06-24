from __future__ import annotations

import hashlib
import os
import re
import tempfile
import types
from collections.abc import Iterator
from pathlib import Path
from typing import IO


class Cache:
    instance: Cache

    @classmethod
    def get_working_instance_if(cls, condition: bool) -> Cache:
        return cls.instance if condition else NullCache()

    class BucketReader:
        def read_parts(self, size: int) -> Iterator[bytes]:
            raise NotImplementedError

        def read_all(self) -> bytes:
            raise NotImplementedError

        def close(self) -> None:
            raise NotImplementedError

        def __enter__(self) -> Cache.BucketReader:
            return self

        def __exit__(
            self,
            exc_type: type[BaseException] | None,
            exc_value: BaseException | None,
            traceback: types.TracebackType | None,
        ) -> None:
            self.close()

    class BucketWriter:
        def write(self, data: bytes) -> None:
            raise NotImplementedError

        def cancel(self) -> None:
            raise NotImplementedError

        def seal(self) -> None:
            raise NotImplementedError

        def __enter__(self) -> Cache.BucketWriter:
            return self

        def __exit__(
            self,
            exc_type: type[BaseException] | None,
            exc_value: BaseException | None,
            traceback: types.TracebackType | None,
        ) -> None:
            if traceback is None:
                self.seal()
            else:
                self.cancel()

    class BucketRef:
        def __init__(self, key: str) -> None:
            self.key = key

        def open_read(self) -> Cache.BucketReader | None:
            raise NotImplementedError

        def open_write(self) -> Cache.BucketWriter:
            raise NotImplementedError

    def get(self, key: str) -> BucketRef:
        raise NotImplementedError


class NullCache(Cache):
    class NullBucketWriter(Cache.BucketWriter):
        def write(self, data: bytes) -> None:
            pass

        def cancel(self) -> None:
            pass

        def seal(self) -> None:
            pass

    class NullBucketRef(Cache.BucketRef):
        def __init__(self, key: str) -> None:
            super().__init__(key)

        def open_read(self) -> Cache.BucketReader | None:
            return None

        def open_write(self) -> Cache.BucketWriter:
            return NullCache.NullBucketWriter()

    def get(self, key: str) -> Cache.BucketRef:
        return NullCache.NullBucketRef(key)


class FilesystemBasedCache(Cache):
    _SUBDIR = 'flatpak-node-generator'
    _KEY_CHAR_ESCAPE_RE = re.compile(r'[^A-Za-z0-9._\-]')

    def __init__(self, cache_root: Path | None = None) -> None:
        self._cache_root = cache_root or self._default_cache_root()

    @staticmethod
    def _escape_key(key: str) -> str:
        return FilesystemBasedCache._KEY_CHAR_ESCAPE_RE.sub(
            lambda m: f'_{ord(m.group()):02X}', key
        )

    class FilesystemBucketReader(Cache.BucketReader):
        def __init__(self, file: IO[bytes]) -> None:
            self.file = file

        def close(self) -> None:
            self.file.close()

        def read_parts(self, size: int) -> Iterator[bytes]:
            while True:
                data = self.file.read(size)
                if not data:
                    break

                yield data

        def read_all(self) -> bytes:
            return self.file.read()

    class FilesystemBucketWriter(Cache.BucketWriter):
        def __init__(self, file: IO[bytes], temp: Path, target: Path) -> None:
            self.file = file
            self.temp = temp
            self.target = target

        def write(self, data: bytes) -> None:
            self.file.write(data)

        def cancel(self) -> None:
            self.file.close()
            self.temp.unlink()

        def seal(self) -> None:
            self.file.close()
            self.temp.rename(self.target)

    class FilesystemBucketRef(Cache.BucketRef):
        def __init__(self, key: str, cache_root: Path) -> None:
            super().__init__(key)
            self._cache_root = cache_root
            self._cache_path = self._cache_root / self._hash_key(key)
            self._legacy_cache_path = (
                self._cache_root / FilesystemBasedCache._escape_key(key)
            )

        @staticmethod
        def _hash_key(key: str) -> str:
            return hashlib.sha256(key.encode('utf-8')).hexdigest()

        def _migrate_cache_path(self) -> None:
            if not self._cache_path.exists() and self._legacy_cache_path.exists():
                try:
                    self._legacy_cache_path.rename(self._cache_path)
                except OSError:
                    pass

        def open_read(self) -> Cache.BucketReader | None:
            self._migrate_cache_path()

            try:
                fp = self._cache_path.open('rb')
            except FileNotFoundError:
                try:
                    fp = self._legacy_cache_path.open('rb')
                except FileNotFoundError:
                    return None

            return FilesystemBasedCache.FilesystemBucketReader(fp)

        def open_write(self) -> Cache.BucketWriter:
            target = self._cache_path
            if not target.parent.exists():
                target.parent.mkdir(exist_ok=True, parents=True)

            fd, temp = tempfile.mkstemp(dir=self._cache_root, prefix='__temp__')
            return FilesystemBasedCache.FilesystemBucketWriter(
                os.fdopen(fd, 'wb'), Path(temp), target
            )

    @classmethod
    def _default_cache_root(cls) -> Path:
        xdg_cache_home = os.environ.get(
            'XDG_CACHE_HOME', os.path.expanduser('~/.cache')
        )
        return Path(xdg_cache_home) / cls._SUBDIR

    def get(self, key: str) -> Cache.BucketRef:
        return FilesystemBasedCache.FilesystemBucketRef(key, self._cache_root)


Cache.instance = NullCache()
