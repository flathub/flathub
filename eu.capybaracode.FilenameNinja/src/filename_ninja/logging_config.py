"""Application logging configuration.

Centralized logging setup so the app can:
- enable/disable logging via settings
- change level via settings
- keep log size bounded via rotation

The app uses Python's standard `logging` module.
"""

from __future__ import annotations

import logging
import os
from logging.handlers import RotatingFileHandler
from pathlib import Path


LOGGER_NAME = "filename_ninja"


def _safe_mkdir(path: Path) -> None:
    try:
        path.mkdir(parents=True, exist_ok=True)
    except Exception:
        # Best-effort: never fail app startup because logging dir couldn't be created.
        import sys, traceback
        print(f"WARNING: Failed to create log folder {path}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)


def default_log_dir() -> Path:
    """Pick an OS-appropriate log folder.

    Priority:
    1) $XDG_STATE_HOME/filename-ninja (Linux)
    2) ~/.local/state/filename-ninja (Linux default)
    3) ~/.filename-ninja (fallback)
    """

    xdg_state = os.environ.get("XDG_STATE_HOME")
    if xdg_state:
        return Path(xdg_state) / "filename-ninja"
    return Path.home() / ".local" / "state" / "filename-ninja"


LOG_FILENAME = "filename-ninja.log"


def default_log_file() -> Path:
    return default_log_dir() / LOG_FILENAME


def resolve_log_file(log_dir: str | None = None) -> Path:
    """Return the log file path, using *log_dir* when non-empty, else the default."""
    if log_dir:
        return Path(log_dir) / LOG_FILENAME
    return default_log_file()


def configure_logging(*, enabled: bool, level: int, log_file: Path | None = None) -> Path | None:
    """(Re)configure application logging.

    When disabled, all handlers for the app logger are removed.
    When enabled, a rotating file handler is installed.

    Rotation defaults: 1 MiB per file, 5 backups.
    """

    logger = logging.getLogger(LOGGER_NAME)
    logger.propagate = False

    # Clear existing handlers to support live reconfiguration.
    for h in list(logger.handlers):
        try:
            h.close()
        except Exception:
            pass
        try:
            logger.removeHandler(h)
        except Exception:
            pass

    if not enabled:
        logger.setLevel(logging.CRITICAL + 1)
        return None

    path = Path(log_file) if log_file is not None else default_log_file()
    _safe_mkdir(path.parent)

    logger.setLevel(int(level))

    handler = RotatingFileHandler(
        filename=str(path),
        maxBytes=1 * 1024 * 1024,
        backupCount=5,
        encoding="utf-8",
    )
    handler.setLevel(int(level))
    handler.setFormatter(
        logging.Formatter(
            fmt="%(asctime)s %(levelname)s [%(name)s] %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S",
        )
    )
    logger.addHandler(handler)

    # Quiet noisy libraries by default.
    logging.getLogger("PySide6").setLevel(logging.WARNING)

    return path


def get_logger(name: str | None = None) -> logging.Logger:
    """Get a namespaced logger.

    `name=None` returns the base app logger.
    """

    if not name:
        return logging.getLogger(LOGGER_NAME)
    return logging.getLogger(f"{LOGGER_NAME}.{name}")

