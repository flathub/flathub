import platform
import os
import sys
import traceback

import logging

from PySide6 import QtCore, QtGui
from PySide6.QtCore import QTimer, QCoreApplication, QElapsedTimer
from PySide6.QtWidgets import QApplication, QSplashScreen
from PySide6.QtGui import QGuiApplication, QIcon

from .ui.main_window import FilenameNinjaApp
from .ui.theme import set_theme
from .config import MainConfig, ThemeMode
from .logging_config import configure_logging, resolve_log_file, get_logger


def _excepthook(exc_type, exc_value, exc_tb):
    """Global exception hook that logs unhandled exceptions before crashing."""
    try:
        log = get_logger("app")
        log.critical(
            "Unhandled exception — application is crashing:\n%s",
            "".join(traceback.format_exception(exc_type, exc_value, exc_tb)),
        )
        # Flush all handlers so the message is written to disk.
        for handler in log.parent.handlers if log.parent else []:
            handler.flush()
        for handler in log.handlers:
            handler.flush()
        root_logger = logging.getLogger("filename_ninja")
        for handler in root_logger.handlers:
            handler.flush()
    except Exception:
        # Cannot use logging here (logging itself may be broken).
        traceback.print_exc(file=sys.stderr)
    # Call the default hook so the traceback still appears on stderr.
    sys.__excepthook__(exc_type, exc_value, exc_tb)

def main():
    QCoreApplication.setOrganizationName("CapybaraCode")
    QCoreApplication.setApplicationName("FilenameNinja")

    # On Linux (Wayland & X11), the desktop environment matches the running
    # window to a .desktop file via its "desktop file name" / WM_CLASS.
    # Setting this *before* constructing QApplication ensures the compositor
    # picks up the correct icon from the .desktop entry.
    if sys.platform.startswith("linux"):
        flatpak_id = os.environ.get("FLATPAK_ID", "").strip()
        QGuiApplication.setDesktopFileName(flatpak_id or "eu.capybaracode.FilenameNinja")

    app = QApplication(sys.argv)

    # Application icon (from Qt resources).
    # This sets the icon used by the window manager/task switcher.
    app.setWindowIcon(QIcon(":/icons/filename_ninja.png"))

    # Apply theme as early as possible.
    cfg = MainConfig()

    # Configure logging as early as possible.
    try:
        level = int(getattr(cfg.logging, "level").value)
        enabled = bool(getattr(cfg.logging, "enabled"))
        log_dir = str(getattr(cfg.logging, "log_dir", ""))
        log_path = configure_logging(enabled=enabled, level=level, log_file=resolve_log_file(log_dir))
        log = get_logger("app")
        if log_path is not None:
            log.info("Logging enabled (%s) -> %s", logging.getLevelName(level), str(log_path))
            log.info(
                "Application starting: Python %s, PySide6/Qt %s, platform=%s %s",
                platform.python_version(),
                QtCore.qVersion(),
                platform.system(),
                platform.release(),
            )
    except Exception:
        # Never fail startup because of logging.
        traceback.print_exc(file=sys.stderr)

    # Install global exception hook so crashes are written to the log file.
    sys.excepthook = _excepthook

    mode = cfg.app.theme_mode
    if mode == ThemeMode.DARK:
        set_theme(app=app, mode="dark")
    elif mode == ThemeMode.LIGHT:
        set_theme(app=app, mode="light")
    else:
        set_theme(app=app, mode="system")

    # Splash screen (optional).
    # When enabled, show the logo during startup and hide it after the main window
    # is shown AND at least 2 seconds have elapsed (i.e., max(loaded_time, 2s)).
    splash: QSplashScreen | None = None
    splash_timer: QElapsedTimer | None = None
    if bool(getattr(cfg.app, "show_splash_screen", True)):
        splash = QSplashScreen(QtGui.QPixmap(":/images/filename_ninja_logo.png"))
        splash.setWindowFlag(QtCore.Qt.WindowType.WindowStaysOnTopHint, True)
        splash.show()
        app.processEvents()
        splash_timer = QElapsedTimer()
        splash_timer.start()

    window = FilenameNinjaApp(cfg)
    if window.main_config.gui.start_maximized:
        window.showMaximized()
    else:
        window.show()

    # Ensure the splash stays up for at least 2 seconds, and until the window is visible.
    if splash is not None and splash_timer is not None:
        def _finish_splash() -> None:
            try:
                splash.finish(window)
                splash.deleteLater()
            except Exception:
                logging.getLogger("filename_ninja").debug("Failed to close splash screen", exc_info=True)

        remaining_ms = max(0, 1500 - int(splash_timer.elapsed()))
        QTimer.singleShot(remaining_ms, _finish_splash)

    timer = QTimer()
    timer.singleShot(0, window.scroll_to_last_path)  # type: ignore[attr-defined]
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
