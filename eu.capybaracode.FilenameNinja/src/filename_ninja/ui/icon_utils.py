from __future__ import annotations

"""Utility for loading icons with theme-name fallbacks and standard-pixmap fallback."""

import logging

from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QStyle, QWidget

log = logging.getLogger("filename_ninja.ui.icon_utils")


def load_icon(
    *theme_names: str,
    standard_pixmap: QStyle.StandardPixmap | None = None,
    resource_path: str | None = None,
    widget: QWidget | None = None,
) -> QIcon:
    """Try each *theme_names* in order, then fall back to *standard_pixmap* or *resource_path*.

    Parameters
    ----------
    *theme_names:
        One or more freedesktop icon-theme names to try via
        :pymethod:`QIcon.fromTheme`, in priority order.
    standard_pixmap:
        Optional :class:`QStyle.StandardPixmap` used as a fallback when no
        theme icon is available.
    resource_path:
        Optional Qt resource path (e.g. ``":/icons/parent_folder.png"``) used
        as a fallback when no theme icon or standard pixmap is available.
    widget:
        Widget whose :pymethod:`style()` is used to resolve *standard_pixmap*.
        Ignored when *standard_pixmap* is ``None``.

    Returns
    -------
    QIcon
        The first non-null icon found, or an empty :class:`QIcon` if every
        source failed.
    """
    for name in theme_names:
        icon = QIcon.fromTheme(name)
        if not icon.isNull():
            return icon

    if standard_pixmap is not None:
        try:
            style = widget.style() if widget is not None else None
            if style is not None:
                icon = style.standardIcon(standard_pixmap)
                if not icon.isNull():
                    return icon
        except Exception:
            log.debug("Failed to load standard pixmap icon", exc_info=True)

    if resource_path is not None:
        icon = QIcon(resource_path)
        if not icon.isNull():
            return icon

    return QIcon()
