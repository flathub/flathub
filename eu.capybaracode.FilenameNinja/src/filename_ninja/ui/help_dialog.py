from __future__ import annotations

import re

from PySide6 import QtCore
from PySide6.QtCore import Qt
from PySide6.QtWidgets import QApplication, QDialog, QTextBrowser, QVBoxLayout, QWidget

from filename_ninja.config import ThemeMode
from filename_ninja.resources import load_resource_text
from filename_ninja.ui.theme import _system_prefers_dark


class HelpDialog(QDialog):
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Help")
        self.resize(980, 760)
        self.setMinimumSize(860, 620)
        # Non-modal: allow interacting with the main window while Help is open.
        self.setModal(False)
        self.setWindowFlags(
            QtCore.Qt.WindowType.Dialog
            | QtCore.Qt.WindowType.CustomizeWindowHint
            | QtCore.Qt.WindowType.WindowTitleHint
            | QtCore.Qt.WindowType.WindowCloseButtonHint
        )

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(10)

        browser = QTextBrowser(self)
        browser.setOpenExternalLinks(True)
        browser.setReadOnly(True)

        html = load_resource_text(":/html/help.html")
        if html:
            html = self._apply_theme_specific_help_screenshots(html)
        browser.setHtml(html or "<p>Help content could not be loaded.</p>")
        layout.addWidget(browser)

    def _apply_theme_specific_help_screenshots(self, html: str) -> str:
        """Switch help screenshots between light and dark variants by active theme."""

        is_dark = self._is_dark_theme_active()

        if is_dark:
            # help_*.png -> help_*_dark.png (only if not already dark)
            return re.sub(
                r'(qrc:/images/help_[^"\']+?)(?<!_dark)\.png',
                r"\1_dark.png",
                html,
            )

        # Light theme: ensure dark variants are not used.
        return re.sub(
            r'(qrc:/images/help_[^"\']+?)_dark\.png',
            r"\1.png",
            html,
        )

    def _is_dark_theme_active(self) -> bool:
        """Resolve active theme using app config + existing robust system detection."""

        main_config = getattr(self.parent(), "main_config", None)
        if main_config is not None:
            mode = getattr(getattr(main_config, "app", None), "theme_mode", None)
            if mode == ThemeMode.DARK:
                return True
            if mode == ThemeMode.LIGHT:
                return False

        app = QApplication.instance()
        if not isinstance(app, QApplication):
            return False
        return _system_prefers_dark(app)
