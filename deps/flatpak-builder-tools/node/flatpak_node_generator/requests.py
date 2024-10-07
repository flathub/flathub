from typing import AsyncIterator, ClassVar

import contextlib

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
        async with aiohttp.ClientSession(raise_for_status=True) as session:
            async with session.get(url) as response:
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

                return
            except Exception:
                if i == Requests.retries:
                    raise

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


class StubRequests(Requests):
    async def _read_parts(
        self, url: str, size: int = DEFAULT_PART_SIZE
    ) -> AsyncIterator[bytes]:
        yield b''

    async def _read_all(self, url: str) -> bytes:
        return b''


Requests.instance = Requests()
