import json
from typing import Any, NamedTuple

from .cache import Cache
from .integrity import Integrity, IntegrityBuilder
from .requests import Requests


class RemoteUrlMetadata(NamedTuple):
    integrity: Integrity
    size: int

    @staticmethod
    def __get_cache_bucket(cachable: bool, kind: str, url: str) -> Cache.BucketRef:
        return Cache.get_working_instance_if(cachable).get(
            f'remote-url-metadata:{kind}:{url}'
        )

    @staticmethod
    def from_json_object(data: Any) -> 'RemoteUrlMetadata':
        return RemoteUrlMetadata(
            integrity=Integrity.from_json_object(data['integrity']),
            size=data['size'],
        )

    @classmethod
    async def get(
        cls, url: str, *, cachable: bool, integrity_algorithm: str = 'sha256'
    ) -> 'RemoteUrlMetadata':
        bucket = cls.__get_cache_bucket(cachable, 'full', url)

        bucket_reader = bucket.open_read()
        if bucket_reader is not None:
            data = json.loads(bucket_reader.read_all())
            return RemoteUrlMetadata.from_json_object(data)

        builder = IntegrityBuilder(integrity_algorithm)
        size = 0

        async for part in Requests.instance.read_parts(url, cachable=False):
            builder.update(part)
            size += len(part)

        metadata = RemoteUrlMetadata(integrity=builder.build(), size=size)

        with bucket.open_write() as bucket_writer:
            bucket_writer.write(json.dumps(metadata.to_json_object()).encode('ascii'))

        return metadata

    @classmethod
    async def get_size(cls, url: str, *, cachable: bool) -> int:
        bucket = cls.__get_cache_bucket(cachable, 'size', url)

        bucket_reader = bucket.open_read()
        if bucket_reader is not None:
            return int(bucket_reader.read_all())

        size = 0
        async for part in Requests.instance.read_parts(url, cachable=False):
            size += len(part)

        with bucket.open_write() as bucket_writer:
            bucket_writer.write(str(size).encode('ascii'))

        return size

    def to_json_object(self) -> Any:
        return {
            'integrity': self.integrity.to_json_object(),
            'size': self.size,
        }
