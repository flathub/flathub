from __future__ import annotations

"""Filesystem rename primitives used by the main window.

Extracted from [`src/filename_ninja/ui/main_window.py`](../main_window.py:1).
"""

import os
from dataclasses import dataclass
from pathlib import Path

from PySide6.QtGui import QUndoCommand

from ...logging_config import get_logger

log = get_logger("rename_ops")


@dataclass(frozen=True)
class RenameOp:
    """A single filesystem rename operation."""

    old_path: Path
    new_path: Path
    label: str


class RenameCommand(QUndoCommand):
    """Undoable rename command.

    QUndoStack will call redo() once when the command is pushed.
    We rename *before* pushing so we can stop cleanly on errors without losing undo history.
    """

    def __init__(self, old_path: Path, new_path: Path, *, label: str) -> None:
        super().__init__(label)
        self._old_path = Path(old_path)
        self._new_path = Path(new_path)
        self._skip_first_redo = True

    def redo(self) -> None:  # type: ignore[override]
        if self._skip_first_redo:
            self._skip_first_redo = False
            return
        rename_path(self._old_path, self._new_path)

    def undo(self) -> None:  # type: ignore[override]
        log.debug("Undo rename: %s -> %s", self._new_path, self._old_path)
        rename_path(self._new_path, self._old_path)


def rename_path(old_path: Path, new_path: Path) -> None:
    """Rename a file/dir, rejecting unsafe overwrites but allowing case-only changes."""

    old_path = Path(old_path)
    new_path = Path(new_path)

    # No-op — use string comparison so that case-only renames (e.g. "Foo" → "foo")
    # are NOT treated as no-ops on platforms where Path.__eq__ is case-insensitive
    # (Windows).
    if str(old_path) == str(new_path):
        return

    log.debug("rename_path: %s -> %s", old_path, new_path)

    # `Path.exists()` follows symlinks and returns False for broken symlinks.
    # Renaming a symlink (even a broken one) is still a valid filesystem operation,
    # so use `lexists` semantics here.
    if not os.path.lexists(str(old_path)):
        raise FileNotFoundError(str(old_path))

    # Allow case-only renames on case-insensitive filesystems.
    if os.path.normcase(str(old_path)) != os.path.normcase(str(new_path)):
        # Same lexists reasoning: don't allow overwriting an existing path entry
        # even if it's a broken symlink.
        if os.path.lexists(str(new_path)):
            raise FileExistsError(str(new_path))

    old_path.rename(new_path)

