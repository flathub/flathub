from __future__ import annotations

"""Regex reference dock used by the main window."""

import logging

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QDockWidget, QMainWindow, QTextBrowser

from filename_ninja.resources import load_resource_text

log = logging.getLogger("filename_ninja.ui.main_window.regex_reference")


def regex_reference_html() -> str:
    html = load_resource_text(":/html/regex_reference.html")
    return html or "<p>Regex reference content could not be loaded.</p>"


def build_regex_reference_dock(main_window: QMainWindow) -> QDockWidget:
    dock = QDockWidget("Regex reference", main_window)
    dock.setObjectName("regex_reference_dock")
    dock.setAllowedAreas(Qt.DockWidgetArea.LeftDockWidgetArea | Qt.DockWidgetArea.RightDockWidgetArea)
    dock.setFeatures(
        QDockWidget.DockWidgetFeature.DockWidgetClosable
        | QDockWidget.DockWidgetFeature.DockWidgetMovable
        | QDockWidget.DockWidgetFeature.DockWidgetFloatable
    )

    browser = QTextBrowser(dock)
    browser.setOpenExternalLinks(True)
    browser.setReadOnly(True)
    browser.setStyleSheet(
        """
        QTextBrowser { padding: 8px; }
        code { font-family: monospace; }
        """
    )
    browser.setHtml(regex_reference_html())

    dock.setWidget(browser)
    main_window.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, dock)
    dock.hide()
    return dock


def toggle_regex_reference_dock(dock: QDockWidget) -> None:
    if dock.isVisible():
        dock.hide()
    else:
        dock.show()
        dock.raise_()
        try:
            dock.setFocus()
        except Exception:
            log.debug("Failed to set focus on regex reference dock", exc_info=True)
