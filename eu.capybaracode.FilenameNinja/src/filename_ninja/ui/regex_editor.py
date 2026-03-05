from __future__ import annotations

import logging

"""Single-line-ish regex editors with syntax highlighting.

We use `QPlainTextEdit` + `QSyntaxHighlighter` because `QLineEdit` cannot do
per-token formatting. The widget is configured to visually behave like a single
line editor (fixed height, no wrapping, no scrollbars) and to reject newlines.

Theme support is handled by deriving colors from the current palette.
"""

log = logging.getLogger("filename_ninja.ui.regex_editor")

from dataclasses import dataclass

from PySide6 import QtCore, QtGui, QtWidgets


@dataclass(frozen=True)
class RegexColors:
    meta: QtGui.QColor
    char_class: QtGui.QColor
    group: QtGui.QColor
    quant: QtGui.QColor
    escape: QtGui.QColor
    anchor: QtGui.QColor
    backref: QtGui.QColor
    repl_ref: QtGui.QColor
    error: QtGui.QColor


def colors_for_palette(*, palette: QtGui.QPalette) -> RegexColors:
    text = palette.color(QtGui.QPalette.ColorRole.Text)
    lum = 0.2126 * text.red() + 0.7152 * text.green() + 0.0722 * text.blue()
    dark = lum > 160  # light text => dark theme

    if dark:
        return RegexColors(
            meta=QtGui.QColor("#D4D4D4"),
            char_class=QtGui.QColor("#4EC9B0"),
            group=QtGui.QColor("#C586C0"),
            quant=QtGui.QColor("#D7BA7D"),
            escape=QtGui.QColor("#9CDCFE"),
            anchor=QtGui.QColor("#569CD6"),
            backref=QtGui.QColor("#CE9178"),
            repl_ref=QtGui.QColor("#CE9178"),
            error=QtGui.QColor("#F44747"),
        )

    return RegexColors(
        meta=QtGui.QColor("#202020"),
        char_class=QtGui.QColor("#00796B"),
        group=QtGui.QColor("#6A1B9A"),
        quant=QtGui.QColor("#8D6E63"),
        escape=QtGui.QColor("#1565C0"),
        anchor=QtGui.QColor("#0D47A1"),
        backref=QtGui.QColor("#AD1457"),
        repl_ref=QtGui.QColor("#AD1457"),
        error=QtGui.QColor("#B71C1C"),
    )


class _BaseRegexHighlighter(QtGui.QSyntaxHighlighter):
    def __init__(self, doc: QtGui.QTextDocument, *, kind: str) -> None:
        super().__init__(doc)
        self._kind = (kind or "pattern").strip().lower()

    def _fmt(self, col: QtGui.QColor, *, bold: bool = False) -> QtGui.QTextCharFormat:
        f = QtGui.QTextCharFormat()
        f.setForeground(col)
        if bold:
            f.setFontWeight(QtGui.QFont.Weight.DemiBold)
        return f

    def _colors(self) -> RegexColors:
        # Palette is attached to the document's default font/palette chain.
        try:
            pal = self.document().defaultFont()._palette  # type: ignore[attr-defined]
        except Exception:
            log.debug("Failed to get palette from document font", exc_info=True)
            pal = None
        # Prefer QApplication palette for correctness.
        app = QtWidgets.QApplication.instance()
        if app is not None:
            pal = app.palette() # type: ignore
        return colors_for_palette(palette=pal or QtGui.QPalette())

    def highlightBlock(self, text: str) -> None:  # type: ignore[override]
        colors = self._colors()
        if self._kind == "replacement":
            _highlight_replacement(self, text, colors)
        else:
            _highlight_pattern(self, text, colors)


def _highlight_pattern(h: _BaseRegexHighlighter, text: str, colors: RegexColors) -> None:
    i = 0
    n = len(text)
    in_class = False
    group_depth = 0

    fmt_escape = h._fmt(colors.escape)
    fmt_class = h._fmt(colors.char_class)
    fmt_class_b = h._fmt(colors.char_class, bold=True)
    fmt_group_b = h._fmt(colors.group, bold=True)
    fmt_quant_b = h._fmt(colors.quant, bold=True)
    fmt_anchor_b = h._fmt(colors.anchor, bold=True)
    fmt_backref_b = h._fmt(colors.backref, bold=True)
    fmt_meta = h._fmt(colors.meta)

    while i < n:
        ch = text[i]

        if ch == "\\" and i + 1 < n:
            nxt = text[i + 1]
            if nxt in ("A", "Z", "z", "b", "B"):
                h.setFormat(i, 2, fmt_anchor_b)
                i += 2
                continue
            if nxt.isdigit():
                j = i + 1
                while j < n and text[j].isdigit():
                    j += 1
                h.setFormat(i, j - i, fmt_backref_b)
                i = j
                continue
            h.setFormat(i, 2, fmt_escape)
            i += 2
            continue

        if ch == "[" and not in_class:
            in_class = True
            h.setFormat(i, 1, fmt_class_b)
            i += 1
            continue
        if ch == "]" and in_class:
            in_class = False
            h.setFormat(i, 1, fmt_class_b)
            i += 1
            continue
        if in_class:
            if ch == "\\" and i + 1 < n:
                h.setFormat(i, 2, fmt_escape)
                i += 2
            else:
                h.setFormat(i, 1, fmt_class)
                i += 1
            continue

        if ch in ("(", ")", "|"):
            if ch == "(":
                group_depth += 1
            elif ch == ")":
                group_depth = max(0, group_depth - 1)
            h.setFormat(i, 1, fmt_group_b)
            i += 1
            continue

        if ch in ("^", "$"):
            h.setFormat(i, 1, fmt_anchor_b)
            i += 1
            continue

        if ch in ("*", "+", "?"):
            h.setFormat(i, 1, fmt_quant_b)
            i += 1
            continue

        if ch == "{" and "}" in text[i:]:
            j = i + 1
            while j < n and text[j] != "}":
                j += 1
            if j < n and text[j] == "}":
                h.setFormat(i, j + 1 - i, fmt_quant_b)
                i = j + 1
                continue

        if ch == ".":
            h.setFormat(i, 1, fmt_meta)
            i += 1
            continue

        i += 1

    # Indicate probable errors by coloring the whole block background lightly.
    if in_class or group_depth:
        f = h._fmt(colors.error, bold=True)
        h.setFormat(0, n, f)


def _highlight_replacement(h: _BaseRegexHighlighter, text: str, colors: RegexColors) -> None:
    i = 0
    n = len(text)

    fmt_escape = h._fmt(colors.escape)
    fmt_ref_b = h._fmt(colors.repl_ref, bold=True)

    while i < n:
        ch = text[i]

        if ch == "\\" and i + 1 < n:
            if text[i + 1].isdigit():
                j = i + 1
                while j < n and text[j].isdigit():
                    j += 1
                h.setFormat(i, j - i, fmt_ref_b)
                i = j
                continue
            if text.startswith("\\g<", i):
                j = i + 3
                while j < n and text[j] != ">":
                    j += 1
                if j < n and text[j] == ">":
                    h.setFormat(i, j + 1 - i, fmt_ref_b)
                    i = j + 1
                    continue
            h.setFormat(i, 2, fmt_escape)
            i += 2
            continue

        if ch == "$" and i + 1 < n:
            if text[i + 1].isdigit():
                j = i + 1
                while j < n and text[j].isdigit():
                    j += 1
                h.setFormat(i, j - i, fmt_ref_b)
                i = j
                continue
            if text.startswith("${", i):
                j = i + 2
                while j < n and text[j] != "}":
                    j += 1
                if j < n and text[j] == "}":
                    h.setFormat(i, j + 1 - i, fmt_ref_b)
                    i = j + 1
                    continue

        i += 1


class _RegexHistoryMenuItemWidget(QtWidgets.QWidget):
    """A single row inside the regex history drop-down: label + remove button."""

    selected = QtCore.Signal(str)
    removed = QtCore.Signal(str)

    def __init__(self, text: str, parent: QtWidgets.QWidget | None = None) -> None:
        super().__init__(parent)
        self._text = text

        layout = QtWidgets.QHBoxLayout(self)
        layout.setContentsMargins(4, 2, 4, 2)
        layout.setSpacing(4)

        self._label = QtWidgets.QLabel(text)
        layout.addWidget(self._label, 1)

        self._remove_btn = QtWidgets.QToolButton()
        self._remove_btn.setText("✕")
        self._remove_btn.setFixedSize(20, 20)
        self._remove_btn.setToolTip("Remove from history")
        self._remove_btn.setFocusPolicy(QtCore.Qt.FocusPolicy.NoFocus)
        self._remove_btn.setStyleSheet(
            "QToolButton { border: none; font-weight: bold; }"
            "QToolButton:hover { color: #e04040; }"
        )
        self._remove_btn.clicked.connect(self._on_remove)
        layout.addWidget(self._remove_btn)

    def _on_remove(self) -> None:
        self.removed.emit(self._text)

    def mousePressEvent(self, event) -> None:  # type: ignore[override]
        if event.button() == QtCore.Qt.MouseButton.LeftButton:
            child = self.childAt(event.pos())
            if child is not self._remove_btn:
                self.selected.emit(self._text)
        super().mousePressEvent(event)


class RegexTextEdit(QtWidgets.QPlainTextEdit):
    """QPlainTextEdit configured to behave like a single-line editor."""

    textChangedSingleLine = QtCore.Signal(str)
    historyItemSelected = QtCore.Signal(str)
    historyItemRemoved = QtCore.Signal(str)

    def __init__(self, parent: QtWidgets.QWidget | None = None, *, kind: str) -> None:
        super().__init__(parent)
        self._kind = (kind or "pattern").strip().lower()
        self._history: list[str] = []

        self.setWordWrapMode(QtGui.QTextOption.WrapMode.NoWrap)
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.setTabChangesFocus(True)
        self.setLineWrapMode(QtWidgets.QPlainTextEdit.LineWrapMode.NoWrap)

        # Visually match QLineEdit sizing.
        self.setFrameShape(QtWidgets.QFrame.Shape.StyledPanel)
        self.setSizePolicy(QtWidgets.QSizePolicy.Policy.Expanding, QtWidgets.QSizePolicy.Policy.Fixed)

        # Syntax highlighter.
        self._highlighter = _BaseRegexHighlighter(self.document(), kind=self._kind)

        # Highlighter should be active only when the widget is enabled.
        self._update_highlighter_enabled()

        # Keep height to 1 line.
        self._sync_height()
        self.document().documentLayout().documentSizeChanged.connect(lambda _s: self._sync_height())

        # Emit plain string.
        self.textChanged.connect(self._emit_single_line)

        # Embedded drop-down button for history.
        self._btn = QtWidgets.QToolButton(self)
        self._btn.setCursor(QtCore.Qt.CursorShape.ArrowCursor)
        self._btn.setArrowType(QtCore.Qt.ArrowType.DownArrow)
        self._btn.setFocusPolicy(QtCore.Qt.FocusPolicy.NoFocus)
        self._btn.setStyleSheet("QToolButton { border: none; padding: 0px; }")
        self._btn.clicked.connect(self._show_history_menu)
        btn_size = 18
        self._btn.setFixedSize(btn_size, btn_size)
        # Reserve space on the right so text doesn't overlap the button.
        self.setViewportMargins(0, 0, btn_size + 2, 0)

    def _update_highlighter_enabled(self) -> None:
        try:
            self._highlighter.setDocument(self.document() if self.isEnabled() else None)
        except Exception:
            log.debug("Failed to toggle highlighter document", exc_info=True)

    def changeEvent(self, event: QtCore.QEvent) -> None:  # type: ignore[override]
        # Toggle syntax highlighting based on enabled state so disabled regex editors
        # render like other disabled text.
        if event.type() == QtCore.QEvent.Type.EnabledChange:
            self._update_highlighter_enabled()
        super().changeEvent(event)

    def _sync_height(self) -> None:
        fm = QtGui.QFontMetrics(self.font())
        line_h = fm.lineSpacing()
        margins = self.contentsMargins()
        # QPlainTextEdit adds some internal padding; approximate via frame width.
        fw = int(self.frameWidth())
        h = int(line_h + margins.top() + margins.bottom() + fw * 2 + 6)
        self.setFixedHeight(h)

    def _emit_single_line(self) -> None:
        self.textChangedSingleLine.emit(self.toPlainText())

    def keyPressEvent(self, event: QtGui.QKeyEvent) -> None:  # type: ignore[override]
        # Reject newline insertion.
        if event.key() in (QtCore.Qt.Key.Key_Return, QtCore.Qt.Key.Key_Enter):
            event.ignore()
            self.clearFocus()
            return
        super().keyPressEvent(event)

    def insertFromMimeData(self, source: QtCore.QMimeData) -> None:  # type: ignore[override]
        # Strip newlines on paste.
        txt = source.text().replace("\r\n", " ").replace("\n", " ").replace("\r", " ")
        self.insertPlainText(txt)

    # ------------------------------------------------------------------
    # History drop-down
    # ------------------------------------------------------------------

    def set_history(self, items: list[str]) -> None:
        """Replace the history list shown in the drop-down menu."""
        self._history = list(items)

    def history(self) -> list[str]:
        """Return the current history list."""
        return list(self._history)

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
        menu = QtWidgets.QMenu(self)
        menu.setMinimumWidth(self.width())
        # Match the menu border color to the input field border.
        border_color = self.palette().color(self.palette().ColorRole.Mid).name()
        menu.setStyleSheet(
            f"QMenu {{ border: 1px solid {border_color}; }}"
        )

        for entry in self._history:
            row = _RegexHistoryMenuItemWidget(entry, menu)
            row.selected.connect(lambda text, m=menu: self._on_history_selected(text, m))
            row.removed.connect(lambda text, m=menu: self._on_history_removed(text, m))
            wa = QtWidgets.QWidgetAction(menu)
            wa.setDefaultWidget(row)
            menu.addAction(wa)

        menu.exec(self.mapToGlobal(self.rect().bottomLeft()))

    def _on_history_selected(self, text: str, menu: QtWidgets.QMenu) -> None:
        menu.close()
        self.setPlainText(text)
        self.historyItemSelected.emit(text)

    def _on_history_removed(self, text: str, menu: QtWidgets.QMenu) -> None:
        try:
            self._history.remove(text)
        except ValueError:
            pass
        self.historyItemRemoved.emit(text)
        # Rebuild the menu so the user can continue removing items.
        menu.close()
        if self._history:
            self._show_history_menu()
