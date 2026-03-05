"""UI-side helpers for log actions."""

from __future__ import annotations

import logging
import os
import subprocess
import sys
from pathlib import Path

from PySide6 import QtWidgets

log = logging.getLogger("filename_ninja.ui.log_utils")


def reveal_in_file_manager(parent: QtWidgets.QWidget | None, path: Path) -> None:
    """Best-effort: open file manager for the log file/folder."""

    try:
        p = Path(path)
    except Exception:
        log.debug("Invalid path for reveal_in_file_manager: %r", path, exc_info=True)
        return

    if p.is_file():
        target = p.parent
    else:
        target = p

    try:
        if sys.platform.startswith("win"):
            os.startfile(str(target))  # type: ignore[attr-defined]
        elif sys.platform == "darwin":
            subprocess.Popen(["open", str(target)])
        else:
            subprocess.Popen(["xdg-open", str(target)])
    except Exception:
        log.debug("Failed to open file manager for %s", target, exc_info=True)
        # If opening fails, copy path to clipboard as fallback.
        try:
            QtWidgets.QApplication.clipboard().setText(str(target))
        except Exception:
            log.debug("Failed to copy path to clipboard", exc_info=True)

