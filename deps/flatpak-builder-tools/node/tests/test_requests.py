from typing import AsyncIterator

from pytest_httpserver import RequestHandler

import pytest

from conftest import RequestsController
from flatpak_node_generator.requests import Requests

_HELLO = '/hello'
_DATA = b'1234567890'
_DATA2 = _DATA + b'ABC'
_PART_SIZE = 3


def _expect_single_hello(requests: RequestsController) -> RequestHandler:
    return requests.server.expect_oneshot_request(_HELLO, 'GET')


async def _read_parts(it: AsyncIterator[bytes]) -> bytes:
    total = b''
    async for part in it:
        assert len(part) <= _PART_SIZE
        total += part

    return total


async def test_read_all(requests: RequestsController) -> None:
    _expect_single_hello(requests).respond_with_data(_DATA)
    assert (await Requests.instance.read_all(requests.url_for(_HELLO))) == _DATA

    # Twice to make sure it's not cached.
    _expect_single_hello(requests).respond_with_data(_DATA2)
    assert (await Requests.instance.read_all(requests.url_for(_HELLO))) == _DATA2


async def test_read_all_retries(requests: RequestsController) -> None:
    assert Requests.retries == 3

    _expect_single_hello(requests).respond_with_data(status=500)
    _expect_single_hello(requests).respond_with_data(status=500)
    _expect_single_hello(requests).respond_with_data(status=500)

    with pytest.raises(Exception, match=r'500'):
        await Requests.instance.read_all(requests.url_for(_HELLO))

    _expect_single_hello(requests).respond_with_data(status=500)
    _expect_single_hello(requests).respond_with_data(status=500)
    _expect_single_hello(requests).respond_with_data(_DATA)

    assert (await Requests.instance.read_all(requests.url_for(_HELLO))) == _DATA


async def test_read_all_cached(requests: RequestsController) -> None:
    _expect_single_hello(requests).respond_with_data(_DATA)

    assert (
        await Requests.instance.read_all(requests.url_for(_HELLO), cachable=True)
    ) == _DATA
    assert (
        await Requests.instance.read_all(requests.url_for(_HELLO), cachable=True)
    ) == _DATA


async def test_read_parts(requests: RequestsController) -> None:
    _expect_single_hello(requests).respond_with_data(_DATA)
    assert (
        await _read_parts(
            Requests.instance.read_parts(requests.url_for(_HELLO), size=_PART_SIZE)
        )
    ) == _DATA

    # Twice to make sure it's not cached.
    _expect_single_hello(requests).respond_with_data(_DATA2)
    assert (
        await _read_parts(
            Requests.instance.read_parts(requests.url_for(_HELLO), size=_PART_SIZE)
        )
    ) == _DATA2


async def test_read_parts_retries(requests: RequestsController) -> None:
    assert Requests.retries == 3

    _expect_single_hello(requests).respond_with_data(status=500)
    _expect_single_hello(requests).respond_with_data(status=500)
    _expect_single_hello(requests).respond_with_data(_DATA)

    assert (
        await _read_parts(
            Requests.instance.read_parts(requests.url_for(_HELLO), size=_PART_SIZE)
        )
    ) == _DATA

    _expect_single_hello(requests).respond_with_data(status=500)
    _expect_single_hello(requests).respond_with_data(status=500)
    _expect_single_hello(requests).respond_with_data(status=500)

    with pytest.raises(Exception, match=r'500'):
        await _read_parts(
            Requests.instance.read_parts(requests.url_for(_HELLO), size=_PART_SIZE)
        )


async def test_read_parts_cached(requests: RequestsController) -> None:
    _expect_single_hello(requests).respond_with_data(_DATA)

    assert (
        await _read_parts(
            Requests.instance.read_parts(
                requests.url_for(_HELLO), size=_PART_SIZE, cachable=True
            )
        )
    ) == _DATA

    assert (
        await _read_parts(
            Requests.instance.read_parts(
                requests.url_for(_HELLO), size=_PART_SIZE, cachable=True
            )
        )
    ) == _DATA
