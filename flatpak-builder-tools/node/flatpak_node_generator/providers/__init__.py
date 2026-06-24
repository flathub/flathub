from __future__ import annotations

import re
import urllib.parse
from collections.abc import Iterator
from contextlib import AbstractContextManager
from pathlib import Path

from ..manifest import ManifestGenerator
from ..node_headers import NodeHeaders
from ..package import GitSource, Package
from .special import SpecialSourceProvider

_GIT_SCHEMES: dict[str, dict[str, str]] = {
    'github': {'scheme': 'https', 'netloc': 'github.com'},
    'gitlab': {'scheme': 'https', 'netloc': 'gitlab.com'},
    'bitbucket': {'scheme': 'https', 'netloc': 'bitbucket.com'},
    'git': {},
    'git+http': {'scheme': 'http'},
    'git+https': {'scheme': 'https'},
    'git+ssh': {'scheme': 'https'},
}


class LockfileProvider:
    def parse_git_source(self, version: str, from_: str | None = None) -> GitSource:
        # https://github.com/microsoft/pyright/issues/1589
        # pyright: reportPrivateUsage=false

        original_url = urllib.parse.urlparse(version)
        assert original_url.scheme and original_url.path and original_url.fragment

        replacements = _GIT_SCHEMES.get(original_url.scheme, {})
        new_url = original_url._replace(fragment='', **replacements)
        # Replace e.g. git:github.com/owner/repo with git://github.com/owner/repo
        if not new_url.netloc:
            path = new_url.path.split('/')
            new_url = new_url._replace(netloc=path[0], path='/'.join(path[1:]))
        # Replace https://git@github.com:ianstormtaylor/to-camel-case.git
        # with    https://git@github.com/ianstormtaylor/to-camel-case.git
        # for git+ssh URLs
        if ':' in new_url.netloc:
            netloc_split = new_url.netloc.split(':', 1)
            new_url = new_url._replace(
                netloc=netloc_split[0], path=f'/{netloc_split[1]}{new_url.path}'
            )

        return GitSource(
            original=original_url.geturl(),
            url=new_url.geturl(),
            commit=original_url.fragment,
            from_=from_,
        )

    def process_lockfile(self, lockfile_path: Path) -> Iterator[Package]:
        raise NotImplementedError()


class RCFileProvider:
    RCFILE_NAME: str

    def parse_rcfile(self, rcfile: Path) -> dict[str, str]:
        with open(rcfile, 'r', encoding='utf-8') as r:
            rcfile_text = r.read()
        parser_re = re.compile(
            r'^(?!#|;)(\S+)(?:\s+|\s*=\s*)(?:"(.+)"|(\S+))$', re.MULTILINE
        )
        result: dict[str, str] = {}
        for key, quoted_val, val in parser_re.findall(rcfile_text):
            result[key] = quoted_val or val
        return result

    def get_node_headers(self, rcfile: Path) -> NodeHeaders | None:
        rc_data = self.parse_rcfile(rcfile)
        if 'target' not in rc_data:
            return None
        target = rc_data['target']
        runtime = rc_data.get('runtime')
        disturl = rc_data.get('disturl')

        assert isinstance(runtime, str) and isinstance(disturl, str)

        return NodeHeaders.with_defaults(target, runtime, disturl)


class ModuleProvider(AbstractContextManager['ModuleProvider']):
    async def generate_package(self, package: Package) -> None:
        raise NotImplementedError()


class ProviderFactory:
    def create_lockfile_provider(self) -> LockfileProvider:
        raise NotImplementedError()

    def create_rcfile_providers(self) -> list[RCFileProvider]:
        raise NotImplementedError()

    def create_module_provider(
        self, gen: ManifestGenerator, special: SpecialSourceProvider
    ) -> ModuleProvider:
        raise NotImplementedError()
