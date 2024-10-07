from pathlib import Path
from typing import (
    Any,
    ContextManager,
    Dict,
    Iterator,
    List,
    Optional,
    Set,
    Tuple,
    Type,
    Union,
)

import base64
import json
import types

from .integrity import Integrity


class ManifestGenerator(ContextManager['ManifestGenerator']):
    MAX_GITHUB_SIZE = 49 * 1000 * 1000
    JSON_INDENT = 4

    def __init__(self) -> None:
        # Store the dicts as a set of tuples, then rebuild the dict when returning it.
        # That way, we ensure uniqueness.
        self._sources: Set[Tuple[Tuple[str, Any], ...]] = set()
        self._commands: List[str] = []

    def __exit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_value: Optional[BaseException],
        tb: Optional[types.TracebackType],
    ) -> None:
        self._finalize()

    @property
    def data_root(self) -> Path:
        return Path('flatpak-node')

    @property
    def tmp_root(self) -> Path:
        return self.data_root / 'tmp'

    @property
    def source_count(self) -> int:
        return len(self._sources)

    def ordered_sources(self) -> Iterator[Dict[Any, Any]]:
        return map(dict, sorted(self._sources))

    def split_sources(self) -> Iterator[List[Dict[Any, Any]]]:
        BASE_CURRENT_SIZE = len('[\n]')
        current_size = BASE_CURRENT_SIZE
        current: List[Dict[Any, Any]] = []

        for source in self.ordered_sources():
            # Generate one source by itself, then check the length without the closing and
            # opening brackets.
            source_json = json.dumps([source], indent=ManifestGenerator.JSON_INDENT)
            source_json_len = len('\n'.join(source_json.splitlines()[1:-1]))
            if current_size + source_json_len >= ManifestGenerator.MAX_GITHUB_SIZE:
                yield current
                current = []
                current_size = BASE_CURRENT_SIZE
            current.append(source)
            current_size += source_json_len

        if current:
            yield current

    def _add_source(self, source: Dict[str, Any]) -> None:
        self._sources.add(tuple(source.items()))

    def _add_source_with_destination(
        self,
        source: Dict[str, Any],
        destination: Optional[Path],
        *,
        is_dir: bool,
        only_arches: Optional[List[str]] = None,
    ) -> None:
        if destination is not None:
            if is_dir:
                source['dest'] = str(destination)
            else:
                source['dest-filename'] = destination.name
                if len(destination.parts) > 1:
                    source['dest'] = str(destination.parent)

        if only_arches:
            source['only-arches'] = tuple(only_arches)

        self._add_source(source)

    def add_local_file_source(
        self,
        path: Path,
        destination: Optional[Path] = None,
        *,
        only_arches: Optional[List[str]] = None,
    ) -> None:
        source: Dict[str, Any] = {
            'type': 'file',
            'path': str(path),
        }
        self._add_source_with_destination(
            source, destination, is_dir=False, only_arches=only_arches
        )

    def add_url_source(
        self,
        url: str,
        integrity: Integrity,
        destination: Optional[Path] = None,
        *,
        only_arches: Optional[List[str]] = None,
    ) -> None:
        source: Dict[str, Any] = {
            'type': 'file',
            'url': url,
            integrity.algorithm: integrity.digest,
        }
        self._add_source_with_destination(
            source, destination, is_dir=False, only_arches=only_arches
        )

    def add_archive_source(
        self,
        url: str,
        integrity: Integrity,
        destination: Optional[Path] = None,
        only_arches: Optional[List[str]] = None,
        strip_components: int = 1,
    ) -> None:
        source: Dict[str, Any] = {
            'type': 'archive',
            'url': url,
            'strip-components': strip_components,
            integrity.algorithm: integrity.digest,
        }
        self._add_source_with_destination(
            source, destination, is_dir=True, only_arches=only_arches
        )

    def add_data_source(self, data: Union[str, bytes], destination: Path) -> None:
        if isinstance(data, bytes):
            source = {
                'type': 'inline',
                'contents': base64.b64encode(data).decode('ascii'),
                'base64': True,
            }
        else:
            assert isinstance(data, str)
            source = {
                'type': 'inline',
                'contents': data,
            }
        self._add_source_with_destination(source, destination, is_dir=False)

    def add_git_source(
        self, url: str, commit: str, destination: Optional[Path] = None
    ) -> None:
        source = {'type': 'git', 'url': url, 'commit': commit}
        self._add_source_with_destination(source, destination, is_dir=True)

    def add_script_source(self, commands: List[str], destination: Path) -> None:
        source = {'type': 'script', 'commands': tuple(commands)}
        self._add_source_with_destination(source, destination, is_dir=False)

    def add_shell_source(
        self,
        commands: List[str],
        destination: Optional[Path] = None,
        only_arches: Optional[List[str]] = None,
    ) -> None:
        """This might be slow for multiple instances. Use `add_command()` instead."""
        source = {'type': 'shell', 'commands': tuple(commands)}
        self._add_source_with_destination(
            source,
            destination=destination,
            only_arches=only_arches,
            is_dir=True,
        )

    def add_command(self, command: str) -> None:
        self._commands.append(command)

    def _finalize(self) -> None:
        if self._commands:
            self._add_source({'type': 'shell', 'commands': tuple(self._commands)})
