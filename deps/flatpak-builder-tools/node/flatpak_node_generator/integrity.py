from typing import Any, NamedTuple, Union

import base64
import binascii
import hashlib


class Integrity(NamedTuple):
    algorithm: str
    digest: str

    @staticmethod
    def parse(value: str) -> 'Integrity':
        algorithm, encoded_digest = value.split('-', 1)
        assert algorithm.startswith('sha'), algorithm
        digest = binascii.hexlify(base64.b64decode(encoded_digest)).decode()

        return Integrity(algorithm, digest)

    @staticmethod
    def from_sha1(sha1: str) -> 'Integrity':
        assert len(sha1) == 40, f'Invalid length of sha1: {sha1}'
        return Integrity('sha1', sha1)

    @staticmethod
    def generate(data: Union[str, bytes], *, algorithm: str = 'sha256') -> 'Integrity':
        builder = IntegrityBuilder(algorithm)
        builder.update(data)
        return builder.build()

    @staticmethod
    def from_json_object(data: Any) -> 'Integrity':
        return Integrity(algorithm=data['algorithm'], digest=data['digest'])

    def to_json_object(self) -> Any:
        return {'algorithm': self.algorithm, 'digest': self.digest}

    def to_base64(self) -> str:
        return base64.b64encode(binascii.unhexlify(self.digest)).decode()


class IntegrityBuilder:
    def __init__(self, algorithm: str = 'sha256') -> None:
        self.algorithm = algorithm
        self._hasher = hashlib.new(algorithm)

    def update(self, data: Union[str, bytes]) -> None:
        data_bytes: bytes
        if isinstance(data, str):
            data_bytes = data.encode()
        else:
            data_bytes = data
        self._hasher.update(data_bytes)

    def build(self) -> Integrity:
        return Integrity(algorithm=self.algorithm, digest=self._hasher.hexdigest())
