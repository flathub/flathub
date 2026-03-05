from __future__ import annotations

import logging
import sys

from PySide6.QtCore import Qt
from PySide6 import QtGui
from PySide6.QtGui import QPalette, QGuiApplication
from PySide6.QtWidgets import QApplication

log = logging.getLogger("filename_ninja.ui.theme")


def _system_prefers_dark(app: QApplication) -> bool:
    """Best-effort check for OS dark-mode preference.

    We only need a robust enough signal to decide whether "system" should map
    to our explicit dark/light themes.

    Strategy (in order):
    1. Qt 6.5+ ``styleHints().colorScheme()`` — the most reliable cross-platform
       API that directly queries the OS preference.
    2. Windows registry fallback — reads the ``AppsUseLightTheme`` value which
       is authoritative on Windows 10/11.
    3. Palette luminance heuristic — inspect the current application palette.
       This is the least reliable at startup because ``standardPalette()`` may
       not reflect the OS dark-mode setting (notably on Windows).
    """

    # --- 1. styleHints().colorScheme() (Qt 6.5+) ---
    try:
        hints = QGuiApplication.styleHints()
        if hints is not None:
            scheme = hints.colorScheme()
            # Qt.ColorScheme.Dark == 2, Light == 1, Unknown == 0
            if scheme == Qt.ColorScheme.Dark:
                log.debug("OS dark-mode detected via styleHints().colorScheme()")
                return True
            if scheme == Qt.ColorScheme.Light:
                log.debug("OS light-mode detected via styleHints().colorScheme()")
                return False
            # Unknown — fall through to next strategy.
    except Exception:
        log.debug("styleHints().colorScheme() unavailable", exc_info=True)

    # --- 2. Windows registry fallback ---
    if sys.platform == "win32":
        try:
            import winreg

            with winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\CurrentVersion\Themes\Personalize",
            ) as key:
                value, _ = winreg.QueryValueEx(key, "AppsUseLightTheme")
                is_dark = value == 0
                log.debug(
                    "OS dark-mode detected via Windows registry: AppsUseLightTheme=%s",
                    value,
                )
                return is_dark
        except Exception:
            log.debug("Windows registry dark-mode check failed", exc_info=True)

    # --- 3. Palette luminance heuristic (least reliable at startup) ---
    try:
        pal = app.palette()
        win = pal.color(QPalette.ColorRole.Window)
        # Perceived luminance heuristic.
        lum = 0.2126 * win.redF() + 0.7152 * win.greenF() + 0.0722 * win.blueF()
        return lum < 0.5
    except Exception:
        log.debug("Failed to detect system dark-mode preference", exc_info=True)
        return False


def _dark_palette() -> QPalette:
    """Reasonable dark palette that works across styles (incl. WSLg)."""

    # Colors tuned to be "dark" without being pure black (better contrast / less harsh).
    # Based loosely on common Qt dark palette examples.
    window = QtGui.QColor(45, 45, 45)
    base = QtGui.QColor(30, 30, 30)
    alt_base = QtGui.QColor(40, 40, 40)
    text = QtGui.QColor(220, 220, 220)
    disabled_text = QtGui.QColor(140, 140, 140)
    button = QtGui.QColor(55, 55, 55)
    button_hover = QtGui.QColor(70, 70, 70)
    button_pressed = QtGui.QColor(35, 35, 35)
    highlight = QtGui.QColor(42, 130, 218)
    link = QtGui.QColor(80, 170, 255)
    border = QtGui.QColor(90, 90, 90)

    p = QPalette()
    p.setColor(QPalette.ColorRole.Window, window)
    p.setColor(QPalette.ColorRole.WindowText, text)
    p.setColor(QPalette.ColorRole.Base, base)
    p.setColor(QPalette.ColorRole.AlternateBase, alt_base)
    p.setColor(QPalette.ColorRole.ToolTipBase, window)
    p.setColor(QPalette.ColorRole.ToolTipText, text)
    p.setColor(QPalette.ColorRole.Text, text)
    p.setColor(QPalette.ColorRole.Button, button)
    p.setColor(QPalette.ColorRole.ButtonText, text)
    p.setColor(QPalette.ColorRole.BrightText, Qt.GlobalColor.red)
    p.setColor(QPalette.ColorRole.Link, link)
    p.setColor(QPalette.ColorRole.LinkVisited, link)
    p.setColor(QPalette.ColorRole.Highlight, highlight)
    p.setColor(QPalette.ColorRole.HighlightedText, QtGui.QColor(0, 0, 0))

    # Some styles use these roles for borders / placeholder backgrounds.
    p.setColor(QPalette.ColorRole.Mid, border)
    p.setColor(QPalette.ColorRole.Dark, QtGui.QColor(25, 25, 25))
    p.setColor(QPalette.ColorRole.Shadow, QtGui.QColor(0, 0, 0))

    # Disabled
    p.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, disabled_text)
    p.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, disabled_text)
    p.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, disabled_text)

    # Stash a few derived colors we also mirror in QSS.
    p._fninja_button_hover = button_hover  # type: ignore[attr-defined]
    p._fninja_button_pressed = button_pressed  # type: ignore[attr-defined]
    p._fninja_border = border  # type: ignore[attr-defined]

    return p


def _dark_stylesheet(palette: QPalette) -> str:
    """Supplement palette with QSS for widgets that ignore palette on some platforms."""

    # Retrieve derived colors (best-effort).
    border = getattr(palette, "_fninja_border", None)
    btn_hover = getattr(palette, "_fninja_button_hover", None)
    btn_pressed = getattr(palette, "_fninja_button_pressed", None)

    border_hex = border.name() if border is not None else "#5A5A5A"
    btn_hover_hex = btn_hover.name() if btn_hover is not None else "#464646"
    btn_pressed_hex = btn_pressed.name() if btn_pressed is not None else "#2E2E2E"

    # Important bits based on reported issues:
    # - enforce QLineEdit/QTableView/QAbstractItemView backgrounds
    # - make checkbox indicator visible
    # - make pressed/checked buttons visually distinct
    return f"""
    /*
       Dark mode uses Fusion + QSS which can change metrics (extra padding/spacing)
       compared to the platform style used in light/system. Normalize a few common
       widget heights so the UI matches light mode more closely.
    */

    /* Table headers: keep consistent height across themes/styles */
    QHeaderView::section {{
        min-height: 20px;
        max-height: 20px;
        padding-top: 0px;
        padding-bottom: 0px;
    }}

    /* Buttons: Fusion defaults can be taller; clamp via padding + min-height */
    QPushButton {{
        min-height: 22px;
        padding-top: 2px;
        padding-bottom: 2px;
    }}
    QLineEdit, QTextEdit, QPlainTextEdit, QComboBox {{
         background-color: #1E1E1E;
         color: #DCDCDC;
         border: 1px solid {border_hex};
         border-radius: 4px;
         padding: 2px 6px;
         selection-background-color: #2A82DA;
         selection-color: #000000;
     }}

    /* SpinBox: only set colors so Fusion keeps drawing native arrows
       with proper hover/press feedback (border/border-radius would
       switch to full QSS rendering and kill native arrow painting). */
    QSpinBox, QAbstractSpinBox {{
         background-color: #1E1E1E;
         color: #DCDCDC;
         selection-background-color: #2A82DA;
         selection-color: #000000;
     }}

    /* Disabled controls: keep background/border, only change text color */
    QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,
    QSpinBox:disabled, QComboBox:disabled, QAbstractSpinBox:disabled {{
        color: palette(disabled, text);
    }}

    QComboBox::drop-down {{
        border: 0px;
        width: 18px;
    }}

    QAbstractItemView, QTableView {{
        background-color: #1E1E1E;
        alternate-background-color: #282828;
        color: #DCDCDC;
        selection-background-color: #2A82DA;
        selection-color: #000000;
        gridline-color: {border_hex};
    }}

    QHeaderView::section {{
        background-color: #2D2D2D;
        color: #DCDCDC;
        border: 1px solid {border_hex};
        padding-left: 6px;
        padding-right: 6px;
    }}

    QCheckBox::indicator {{
        width: 14px;
        height: 14px;
        border: 1px solid {border_hex};
        background-color: #1E1E1E;
    }}
    QCheckBox::indicator:checked {{
        background-color: #2A82DA;
        border: 1px solid #2A82DA;
        /* Use Qt built-in indicator image for the checkmark */
        image: url(:/qt-project.org/styles/commonstyle/images/checkbox_checked.png);
    }}
    QCheckBox::indicator:checked:disabled {{
        background-color: #3A3A3A;
        border: 1px solid {border_hex};
        image: url(:/qt-project.org/styles/commonstyle/images/checkbox_checked_disabled.png);
    }}

    /* Make sure the checkmark is visible (size + centering) */
    QCheckBox::indicator:checked, QCheckBox::indicator:checked:disabled {{
        image-position: center;
    }}

    QPushButton {{
        border: 1px solid {border_hex};
        padding-left: 10px;
        padding-right: 10px;
        border-radius: 4px;
    }}
    QPushButton:disabled {{
        color: palette(disabled, button-text);
    }}
    QPushButton:hover {{
        background-color: {btn_hover_hex};
    }}
    QPushButton:pressed {{
        background-color: {btn_pressed_hex};
    }}
    QPushButton:checked {{
        background-color: {btn_pressed_hex};
    }}

    """


def _light_palette() -> QPalette:
    """Reasonable light palette for forcing a light look on platforms where
    the native style follows OS dark mode (notably Windows).

    This is intentionally simple and avoids relying on the platform palette.
    """

    window = QtGui.QColor(245, 245, 245)
    base = QtGui.QColor(255, 255, 255)
    alt_base = QtGui.QColor(240, 240, 240)
    text = QtGui.QColor(20, 20, 20)
    disabled_text = QtGui.QColor(120, 120, 120)
    button = QtGui.QColor(245, 245, 245)
    highlight = QtGui.QColor(42, 130, 218)
    link = QtGui.QColor(0, 102, 204)
    border = QtGui.QColor(190, 190, 190)

    p = QPalette()
    p.setColor(QPalette.ColorRole.Window, window)
    p.setColor(QPalette.ColorRole.WindowText, text)
    p.setColor(QPalette.ColorRole.Base, base)
    p.setColor(QPalette.ColorRole.AlternateBase, alt_base)
    p.setColor(QPalette.ColorRole.ToolTipBase, base)
    p.setColor(QPalette.ColorRole.ToolTipText, text)
    p.setColor(QPalette.ColorRole.Text, text)
    p.setColor(QPalette.ColorRole.Button, button)
    p.setColor(QPalette.ColorRole.ButtonText, text)
    p.setColor(QPalette.ColorRole.Link, link)
    p.setColor(QPalette.ColorRole.LinkVisited, link)
    p.setColor(QPalette.ColorRole.Highlight, highlight)
    p.setColor(QPalette.ColorRole.HighlightedText, QtGui.QColor(255, 255, 255))

    p.setColor(QPalette.ColorRole.Mid, border)
    p.setColor(QPalette.ColorRole.Dark, QtGui.QColor(160, 160, 160))
    p.setColor(QPalette.ColorRole.Shadow, QtGui.QColor(0, 0, 0))

    p.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, disabled_text)
    p.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, disabled_text)
    p.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, disabled_text)

    return p


def _light_stylesheet() -> str:
    """Light-mode QSS.

    This is mainly to override per-widget QSS used for compact spacing that may
    have hard-coded dark colors on some platforms.
    """

    return """
    QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QComboBox, QAbstractSpinBox {
        background-color: #FFFFFF;
        color: #141414;
    }
    QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,
    QSpinBox:disabled, QComboBox:disabled, QAbstractSpinBox:disabled {
        color: palette(disabled, text);
    }
    QAbstractItemView, QTableView {
        background-color: #FFFFFF;
        alternate-background-color: #F0F0F0;
        color: #141414;
        selection-background-color: #2A82DA;
        selection-color: #FFFFFF;
    }
    QHeaderView::section {
        background-color: #F5F5F5;
        color: #141414;
    }

    /* Checkbox: ensure border is visible against light backgrounds (Windows) */
    QCheckBox::indicator {
        width: 14px;
        height: 14px;
        border: 1px solid #888888;
        background-color: #FFFFFF;
    }
    QCheckBox::indicator:checked {
        background-color: #2A82DA;
        border: 1px solid #2A82DA;
        image: url(:/qt-project.org/styles/commonstyle/images/checkbox_checked.png);
    }
    QCheckBox::indicator:disabled {
        border: 1px solid #BEBEBE;
        background-color: #F0F0F0;
    }
    QCheckBox::indicator:checked:disabled {
        background-color: #C0C0C0;
        border: 1px solid #BEBEBE;
        image: url(:/qt-project.org/styles/commonstyle/images/checkbox_checked_disabled.png);
    }
    """


def set_theme(*, app: QApplication, mode: str) -> None:
    """Apply theme.

    mode: one of: "system", "light", "dark"
    """

    m = (mode or "system").strip().lower()

    # Important on Windows: leaving the app style as "" (platform style) can keep
    # a cached dark palette when toggling from dark -> light/system, especially
    # on Win11 when the OS theme is dark. Force a style repolish by explicitly
    # re-applying the *current* platform style name after resetting palette/QSS.
    # This is a no-op on platforms where style refresh works already.
    def _reapply_platform_style() -> None:
        try:
            style_obj = app.style()
            style_name = style_obj.objectName() if style_obj is not None else ""
            if style_name:
                app.setStyle(style_name)
        except Exception:
            log.debug("Failed to reset app style", exc_info=True)

    if m == "dark":
        app.setStyle("Fusion")
        pal = _dark_palette()
        app.setPalette(pal)
        app.setStyleSheet(_dark_stylesheet(pal))
        return

    if m == "light":
        # Force a light look even if the native/platform style is currently in
        # dark mode due to OS settings (common on Windows).
        app.setStyle("Fusion")
        app.setPalette(_light_palette())
        # Ensure widgets with their own QSS still render as light.
        app.setStyleSheet(_light_stylesheet())
        return

    # system: map to *either* our explicit dark or explicit light theme so the
    # app only ever has one dark and one light appearance.
    #
    # On Windows, the native style can produce a *different* dark palette than
    # our explicit dark theme; users expect "system" to be equivalent to dark
    # when the OS is dark.
    #
    # Before probing the OS preference we must temporarily reset the palette and
    # stylesheet to the platform defaults.  Otherwise _system_prefers_dark()
    # would just see the palette we previously forced (dark or light) and always
    # return the *old* theme instead of the real OS preference.
    app.setStyleSheet("")
    app.setPalette(app.style().standardPalette())

    if _system_prefers_dark(app):
        app.setStyle("Fusion")
        pal = _dark_palette()
        app.setPalette(pal)
        app.setStyleSheet(_dark_stylesheet(pal))
    else:
        app.setStyle("Fusion")
        app.setPalette(_light_palette())
        app.setStyleSheet(_light_stylesheet())
