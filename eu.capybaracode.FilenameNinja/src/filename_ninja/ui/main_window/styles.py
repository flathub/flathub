from __future__ import annotations

"""Reusable style helpers for the main window UI."""

import logging

from PySide6.QtGui import QFontMetrics
from PySide6.QtWidgets import QApplication, QComboBox, QLineEdit, QSizePolicy, QSpinBox, QTableView, QWidget

from ..regex_editor import RegexTextEdit

log = logging.getLogger("filename_ninja.ui.main_window.styles")

# ---------------------------------------------------------------------------
# Font-metric-based compact height
# ---------------------------------------------------------------------------

_cached_compact_height: int | None = None


def compact_widget_height() -> int:
    """Return a compact widget height derived from the application font metrics.

    The value is computed once and cached for the lifetime of the process.
    It replaces the former hard-coded ``22`` px constant so that the UI
    scales correctly across different fonts and DPI settings.

    The height is chosen to accommodate the tallest common input widget
    (``QSpinBox``, ``QComboBox``, ``QLineEdit``, ``RegexTextEdit``) so
    that all rule rows share a uniform height.
    """
    global _cached_compact_height
    if _cached_compact_height is not None:
        return _cached_compact_height

    try:
        app = QApplication.instance()
        if app is not None:
            fm = QFontMetrics(app.font())  # type: ignore[arg-type]
        else:
            fm = QFontMetrics(QLineEdit().font())

        # Probe the natural sizeHint of the tallest common widgets so we
        # pick a height that none of them will be clipped at.
        probe_heights: list[int] = [fm.height() + 10]
        for WidgetCls in (QLineEdit, QSpinBox, QComboBox):
            try:
                w = WidgetCls()
                probe_heights.append(w.sizeHint().height())
                w.deleteLater()
            except Exception:
                pass

        h = max(probe_heights)
        _cached_compact_height = max(h, 22)  # never go below 22
    except Exception:
        log.debug("Failed to compute compact widget height", exc_info=True)
        _cached_compact_height = 22

    return _cached_compact_height


# ---------------------------------------------------------------------------
# Style helpers
# ---------------------------------------------------------------------------


def apply_compact_table_style(table: QTableView) -> None:
    """Make table rows more compact by reducing default cell padding."""

    table.setStyleSheet(
        """
        QTableView::item {
            padding-top: 0px;
            padding-bottom: 0px;
            padding-left: 2px;
            padding-right: 2px;
        }
        """
    )

    try:
        vh = table.verticalHeader()
        vh.setDefaultSectionSize(20)
        vh.setMinimumSectionSize(16)
    except Exception:
        log.debug("Failed to set compact table header sizes", exc_info=True)


def apply_compact_line_edit_style(le: QLineEdit) -> None:
    """Make QLineEdit widgets shorter (less vertical padding)."""

    le.setStyleSheet(
        """
        QLineEdit {
            padding-top: 0px;
            padding-bottom: 0px;
            padding-left: 4px;
            padding-right: 4px;
        }
        """
    )

    try:
        le.setFixedHeight(compact_widget_height())
    except Exception:
        log.debug("Failed to set line edit fixed height", exc_info=True)


def apply_compact_regex_text_edit_style(te: RegexTextEdit) -> None:
    """Make RegexTextEdit match compact rule-row height."""

    te.setStyleSheet(
        """
        QPlainTextEdit {
            padding-top: 0px;
            padding-bottom: 0px;
            padding-left: 4px;
            padding-right: 4px;
        }
        """
    )
    try:
        te._sync_height()  # type: ignore[attr-defined]
    except Exception:
        log.debug("Failed to sync RegexTextEdit height", exc_info=True)


def apply_uniform_row_height(row: QWidget, *, height: int | None = None) -> None:
    """Normalize row widget height inside rules tabs.

    When *height* is ``None`` (the default) the value from
    :func:`compact_widget_height` is used.  Every row gets the same
    fixed height so that all rule rows look uniform.
    """

    effective_height = int(height if height is not None else compact_widget_height())

    # If the row contains a RegexTextEdit whose fixed height (set by
    # _sync_height) exceeds the compact height, expand to accommodate it.
    _QT_MAX = 16777215
    try:
        for child in row.findChildren(RegexTextEdit):
            child_h = child.maximumHeight()
            if effective_height < child_h < _QT_MAX:
                effective_height = int(child_h)
    except Exception:
        log.debug("Failed to query RegexTextEdit heights", exc_info=True)

    try:
        row.setMinimumHeight(effective_height)
    except Exception:
        log.debug("Failed to set row minimum height", exc_info=True)
        return

    try:
        row.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        row.setFixedHeight(effective_height)
    except Exception:
        log.debug("Failed to set row size policy", exc_info=True)


def apply_compact_combo_box_style(combo: QComboBox) -> None:
    """Make QComboBox height consistent with compact rule rows."""

    try:
        combo.setFixedHeight(compact_widget_height())
    except Exception:
        log.debug("Failed to set combo box fixed height", exc_info=True)
