import contextlib
import hashlib
from collections.abc import AsyncIterator
from typing import Any, ClassVar

import aiohttp

from .cache import Cache

DEFAULT_PART_SIZE = 4096


class Requests:
    instance: 'Requests'

    DEFAULT_RETRIES = 5
    retries: ClassVar[int] = DEFAULT_RETRIES

    def __get_cache_bucket(self, cachable: bool, url: str) -> Cache.BucketRef:
        return Cache.get_working_instance_if(cachable).get(f'requests:{url}')

    @contextlib.asynccontextmanager
    async def _open_stream(self, url: str) -> AsyncIterator[aiohttp.StreamReader]:
        async with (
            aiohttp.ClientSession(raise_for_status=True) as session,
            session.get(url) as response,
        ):
            yield response.content

    async def _read_parts(
        self, url: str, size: int = DEFAULT_PART_SIZE
    ) -> AsyncIterator[bytes]:
        async with self._open_stream(url) as stream:
            while True:
                data = await stream.read(size)
                if not data:
                    return

                yield data

    async def _read_all(self, url: str) -> bytes:
        async with self._open_stream(url) as stream:
            return await stream.read()

    async def read_parts(
        self, url: str, *, cachable: bool = False, size: int = DEFAULT_PART_SIZE
    ) -> AsyncIterator[bytes]:
        bucket = self.__get_cache_bucket(cachable, url)

        bucket_reader = bucket.open_read()
        if bucket_reader is not None:
            for part in bucket_reader.read_parts(size):
                yield part

            return

        for i in range(1, Requests.retries + 1):
            try:
                with bucket.open_write() as bucket_writer:
                    async for part in self._read_parts(url, size):
                        bucket_writer.write(part)
                        yield part
            except Exception:
                if i == Requests.retries:
                    raise
            else:
                return

    async def read_all(self, url: str, *, cachable: bool = False) -> bytes:
        bucket = self.__get_cache_bucket(cachable, url)

        bucket_reader = bucket.open_read()
        if bucket_reader is not None:
            return bucket_reader.read_all()

        for i in range(1, Requests.retries + 1):
            try:
                with bucket.open_write() as bucket_writer:
                    data = await self._read_all(url)
                    bucket_writer.write(data)
                    return data
            except Exception:
                if i == Requests.retries:
                    raise

        assert False

    async def upgrade_to_sha256(self, sources: list[dict[str, Any]]) -> None:
        for source in sources:
            if 'sha1' in source and 'url' in source:
                url = source['url']
                try:
                    data = await self.read_all(url, cachable=True)
                except (aiohttp.ClientError, OSError):
                    continue
                if data:
                    sha256_digest = hashlib.sha256(data).hexdigest()
                    source['sha256'] = sha256_digest
                    del source['sha1']


class StubRequests(Requests):
    async def _read_parts(
        self, url: str, size: int = DEFAULT_PART_SIZE
    ) -> AsyncIterator[bytes]:
        yield b''

    async def _read_all(self, url: str) -> bytes:
        return b''


Requests.instance = Requests()
