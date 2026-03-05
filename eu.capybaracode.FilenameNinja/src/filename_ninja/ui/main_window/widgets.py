from __future__ import annotations

"""Small widget subclasses used by the main window."""

import time

from PySide6 import QtCore
from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QAction
from PySide6.QtWidgets import (
    QComboBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMenu,
    QToolButton,
    QWidget,
    QWidgetAction,
)


class AutoCloseComboBox(QComboBox):
    """QComboBox variant that tracks popup-open cycles.

    This is used to make popup-closing logic robust on WSLg/Wayland where
    `hidePopup()` can be delayed and otherwise closes the *next* popup opening.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._popup_open_seq: int = 0
        self._popup_open_time_s: float = 0.0

    def showPopup(self) -> None:  # type: ignore[override]
        self._popup_open_seq += 1
        self._popup_open_time_s = time.monotonic()
        super().showPopup()


class _HistoryMenuItemWidget(QWidget):
    """A single row inside the history drop-down: label + remove button."""

    selected = Signal(str)
    removed = Signal(str)

    def __init__(self, text: str, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._text = text

        layout = QHBoxLayout(self)
        layout.setContentsMargins(4, 2, 4, 2)
        layout.setSpacing(4)

        self._label = QLabel(text)
        self._label.setSizePolicy(
            self._label.sizePolicy().horizontalPolicy(),
            self._label.sizePolicy().verticalPolicy(),
        )
        layout.addWidget(self._label, 1)

        self._remove_btn = QToolButton()
        self._remove_btn.setText("✕")
        self._remove_btn.setFixedSize(20, 20)
        self._remove_btn.setToolTip("Remove from history")
        self._remove_btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        self._remove_btn.setStyleSheet(
            "QToolButton { border: none; font-weight: bold; }"
            "QToolButton:hover { color: #e04040; }"
        )
        self._remove_btn.clicked.connect(self._on_remove)
        layout.addWidget(self._remove_btn)

    def _on_remove(self) -> None:
        self.removed.emit(self._text)

    def mousePressEvent(self, event) -> None:  # type: ignore[override]
        # Only select when clicking on the label area, not the remove button.
        if event.button() == Qt.MouseButton.LeftButton:
            child = self.childAt(event.pos())
            if child is not self._remove_btn:
                self.selected.emit(self._text)
        super().mousePressEvent(event)


class HistoryLineEdit(QLineEdit):
    """QLineEdit with a drop-down button that shows a menu of recent entries.

    The widget embeds a small tool-button on the right side of the line edit.
    Clicking the button opens a popup menu listing the items from
    :pymethod:`set_history`.  Selecting an item sets the text and emits
    :pysignal:`returnPressed` so the caller can react immediately.
    """

    # Emitted when the user picks an item from the history menu.
    historyItemSelected = Signal(str)
    # Emitted when the user removes an item from the history menu.
    historyItemRemoved = Signal(str)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._history: list[str] = []

        # Embedded drop-down button
        self._btn = QToolButton(self)
        self._btn.setCursor(Qt.CursorShape.ArrowCursor)
        self._btn.setArrowType(Qt.ArrowType.DownArrow)
        self._btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        self._btn.setStyleSheet("QToolButton { border: none; padding: 0px; }")
        self._btn.clicked.connect(self._show_history_menu)

        # Reserve space on the right for the button.
        btn_size = 18
        self._btn.setFixedSize(btn_size, btn_size)
        self.setTextMargins(0, 0, btn_size + 2, 0)

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def set_history(self, items: list[str]) -> None:
        """Replace the history list shown in the drop-down menu."""
        self._history = list(items)

    def history(self) -> list[str]:
        """Return the current history list."""
        return list(self._history)

    # ------------------------------------------------------------------
    # Internal
    # ------------------------------------------------------------------

    def resizeEvent(self, event) -> None:  # type: ignore[override]
        super().resizeEvent(event)
        # Position the button at the right edge, vertically centred.
        btn_size = self._btn.size()
        self._btn.move(
            self.width() - btn_size.width() - 2,
            (self.height() - btn_size.height()) // 2,
        )

    def _show_history_menu(self) -> None:
        if not self._history:
            return
        menu = QMenu(self)
        menu.setMinimumWidth(self.width())
        # Match the menu border color to the input field border.
        border_color = self.palette().color(self.palette().ColorRole.Mid).name()
        menu.setStyleSheet(
            f"QMenu {{ border: 1px solid {border_color}; }}"
        )

        for entry in self._history:
            row = _HistoryMenuItemWidget(entry, menu)
            row.selected.connect(lambda text, m=menu: self._on_history_selected(text, m))
            row.removed.connect(lambda text, m=menu: self._on_history_removed(text, m))
            wa = QWidgetAction(menu)
            wa.setDefaultWidget(row)
            menu.addAction(wa)

        menu.exec(self.mapToGlobal(self.rect().bottomLeft()))

    def _on_history_selected(self, text: str, menu: QMenu) -> None:
        menu.close()
        self.setText(text)
        self.historyItemSelected.emit(text)
        self.returnPressed.emit()

    def _on_history_removed(self, text: str, menu: QMenu) -> None:
        try:
            self._history.remove(text)
        except ValueError:
            pass
        self.historyItemRemoved.emit(text)
        # Rebuild the menu in-place so the user can continue removing items.
        menu.close()
        if self._history:
            self._show_history_menu()

