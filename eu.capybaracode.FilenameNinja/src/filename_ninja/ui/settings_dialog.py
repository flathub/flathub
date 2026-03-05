from dataclasses import dataclass
import logging
from typing import Any, Callable, Generic, TypeVar

import time

from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QLineEdit,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from ..config import AutoNumberChange, CaseChange, LogLevel, MainConfig, ThemeMode
from ..logging_config import configure_logging, default_log_dir, get_logger, resolve_log_file
from .shortcut_helper import fix_label_buddy_for_mac, shortcut_key
from .theme import set_theme

log = get_logger("settings_dialog")


T = TypeVar("T")


@dataclass(frozen=True)
class _Binding(Generic[T]):
    target: object
    attr: str
    widget: QWidget
    read: Callable[[QWidget], T]
    write: Callable[[QWidget, T], None]


def _enum_label(name: str) -> str:
    return name.replace("_", " ").title()


class SettingsDialog(QDialog):
    """Dialog for editing all persisted settings in `MainConfig`.

    Settings are applied immediately on change.
    """

    def __init__(self, parent: QWidget | None, config: MainConfig) -> None:
        super().__init__(parent)
        self._config = config
        self._bindings: list[_Binding[Any]] = []

        self.setWindowTitle("Settings")
        # Non-modal: allow interacting with the main window while Settings is open.
        self.setModal(False)
        # Show only the close button (no minimize/maximize).
        # CustomizeWindowHint: we control exactly which buttons appear.
        # MSWindowsFixedSizeDialogHint: disables maximize on Windows.
        self.setWindowFlags(
            QtCore.Qt.WindowType.Dialog
            | QtCore.Qt.WindowType.CustomizeWindowHint
            | QtCore.Qt.WindowType.WindowTitleHint
            | QtCore.Qt.WindowType.WindowCloseButtonHint
            | QtCore.Qt.WindowType.MSWindowsFixedSizeDialogHint
        )

        # Ensure the dialog is not resizable/maximizable.
        self.setSizeGripEnabled(False)
        # Avoid passing invalid/negative sizes to QWidget::setMinimumSize.
        # Under some styles/platforms, `sizeHint()` can report (-1, -1) before
        # the layout is fully established, which triggers:
        #   QWidget::setMinimumSize: (/SettingsDialog) Negative sizes (-1,-1)
        # We defer sizing until the dialog is shown and the layout has a
        # deterministic size hint.
        self._fixed_size_applied = False

        # Alt+Q (Ctrl+Q / ⌘Q on macOS) closes the settings dialog
        # (same shortcut that opens it).
        close_shortcut = QtGui.QShortcut(shortcut_key("Q"), self)
        close_shortcut.activated.connect(self.close)

        root = QVBoxLayout(self)  # type: ignore

        # Single settings form (no tabs / no section separators).
        form_root = QWidget(self)
        form = QFormLayout(form_root)  # type: ignore
        root.addWidget(form_root)

        # Theme
        self._add_enum(form, "Theme", self._config.app, "theme_mode", ThemeMode)

        # Path + file mask are controlled directly from the main window.
        self._add_bool(form, "Remember last path", self._config.app, "remember_last_path")
        self._add_bool(form, "Remember position and size", self._config.gui, "remember_position_and_size")
        self._add_bool(form, "Start maximized", self._config.gui, "start_maximized")
        self._add_bool(form, "Show splash screen at startup", self._config.app, "show_splash_screen")
        self._add_bool(form, "&Uncheck after renaming", self._config.app, "uncheck_after_renaming")
        self._add_bool(form, "Ask confirmation before renaming", self._config.app, "ask_confirmation_before_renaming")
        self._add_bool(form, "&File mask case sensitive", self._config.app, "file_mask_case_sensitive")

        # Text-to-replace case sensitivity (Name/Extension tabs)
        self._add_bool(form, "&Name: text to replace case sensitive", self._config.name, "text_to_replace_case_sensitive")
        self._add_bool(form, "&Extension: text to replace case sensitive", self._config.ext, "text_to_replace_case_sensitive")

        # Advanced / hidden options
        self._cb_enable_regex_rename = self._add_bool(form, "Enable regex rename (advanced)", self._config.app, "enable_regex_rename_option")

        # Logging (after other settings)
        self._cb_enable_logging = self._add_bool(form, "Enable &logging", self._config.logging, "enabled")
        self._combo_log_level = self._add_enum(form, "Log level", self._config.logging, "level", LogLevel)

        # Log folder chooser.
        log_dir_row = QWidget(form.parentWidget())
        log_dir_layout = QtWidgets.QHBoxLayout(log_dir_row)  # type: ignore
        log_dir_layout.setContentsMargins(0, 0, 0, 0)
        log_dir_layout.setSpacing(8)
        self._log_dir_le = QLineEdit(log_dir_row)
        self._log_dir_le.setReadOnly(True)
        configured_dir = str(getattr(self._config.logging, "log_dir", ""))
        initial_dir = configured_dir if configured_dir else str(default_log_dir())
        self._log_dir_le.setText(initial_dir)
        self._log_dir_le.setToolTip(initial_dir)
        self._log_dir_le.setPlaceholderText(str(default_log_dir()))
        # Keep tooltip in sync whenever the displayed path changes.
        self._log_dir_le.textChanged.connect(self._log_dir_le.setToolTip)
        log_dir_layout.addWidget(self._log_dir_le, 1)
        self._btn_browse_log_dir = QtWidgets.QPushButton("Browse…", log_dir_row)
        self._btn_browse_log_dir.clicked.connect(self._browse_log_dir)
        log_dir_layout.addWidget(self._btn_browse_log_dir)
        form.addRow("Log folder", log_dir_row)

        # Keep logging-related controls visually in sync with "Enable logging".
        self._log_level_label = self._label_widget_for_field(form, self._combo_log_level)
        self._log_dir_label = self._label_widget_for_field(form, log_dir_row)
        self._sync_logging_controls_enabled()
        self._cb_enable_logging.toggled.connect(lambda _v: self._sync_logging_controls_enabled())
        self._cb_enable_logging.toggled.connect(lambda _v: QtCore.QTimer.singleShot(0, self._sync_logging_controls_enabled))

        # Apply-on-change wiring after all bindings are created.
        self._wire_apply_on_change()

        # If parent (main window) provides a live hook, call it so UI updates
        # immediately while the Settings dialog is open.
        try:
            if hasattr(parent, "_sync_regex_rows_visibility") and self._cb_enable_regex_rename is not None:
                self._cb_enable_regex_rename.toggled.connect(lambda _v: getattr(parent, "_sync_regex_rows_visibility")())
        except Exception:
            pass

        # (Sizing handled above, before layout finishes wiring.)

    def _label_widget_for_field(self, form: QFormLayout, field: QWidget) -> QtWidgets.QLabel | None:
        """Return the QLabel used as the form label for a given field widget."""

        try:
            pos = form.getWidgetPosition(field)
            row = int(getattr(pos, "row", -1))
            if row < 0:
                return None
            item = form.itemAt(row, QFormLayout.ItemRole.LabelRole)
            w = item.widget() if item is not None else None
            return w if isinstance(w, QtWidgets.QLabel) else None
        except Exception:
            return None

    def _sync_logging_controls_enabled(self) -> None:
        enabled = bool(getattr(self._config.logging, "enabled"))
        try:
            self._combo_log_level.setEnabled(enabled)
        except Exception:
            pass
        for w in (
            self._log_dir_le,
            self._btn_browse_log_dir,
        ):
            try:
                w.setEnabled(enabled)
            except Exception:
                pass
        # Also disable the *labels* so they pick up disabled label palette color.
        for lab in (getattr(self, "_log_level_label", None), getattr(self, "_log_dir_label", None)):
            if lab is None:
                continue
            try:
                lab.setEnabled(enabled)
            except Exception:
                pass

    def _browse_log_dir(self) -> None:
        """Open a folder chooser for the log folder."""
        current = self._log_dir_le.text() or str(default_log_dir())
        chosen = QtWidgets.QFileDialog.getExistingDirectory(
            self, "Select log folder", current,
        )
        if chosen:
            self._log_dir_le.setText(chosen)
            self._config.logging.log_dir = chosen
            self._sync_parent_geometry_to_config()
            self._config.save_settings()
            # Reconfigure logging with the new folder.
            self._apply_logging_config()

    def showEvent(self, event: QtGui.QShowEvent) -> None:  # type: ignore[override]
        super().showEvent(event)

        # Apply fixed size once, after the widget is shown and layouts have run.
        if self._fixed_size_applied:
            return

        try:
            self.adjustSize()
            hint = self.sizeHint()
            w = max(0, int(hint.width()))
            h = max(0, int(hint.height()))
            if w > 0 and h > 0:
                self.setFixedSize(w, h)
                self._fixed_size_applied = True
        except Exception:
            # Best-effort: if sizing fails, keep the dialog resizable rather
            # than causing warnings.
            pass

    def changeEvent(self, event: QtCore.QEvent) -> None:  # type: ignore[override]
        # Some compositors still allow minimize/maximize even if the decorations
        # show those buttons. Force-disable those states if they get activated.
        if event.type() == QtCore.QEvent.Type.WindowStateChange:
            st = self.windowState()
            bad = QtCore.Qt.WindowState.WindowMaximized | QtCore.Qt.WindowState.WindowMinimized
            if st & bad:
                self.setWindowState(st & ~bad)
        super().changeEvent(event)

    def _close_combo_popup_on_selection(self, combo: QComboBox) -> None:
        """Ensure QComboBox popup closes immediately after a user selection.

        Mirrors the robust popup-closing logic used in the main window to avoid
        stuck theme dropdowns on some styles/platforms (e.g. WSLg/Wayland).
        """

        def _schedule_hide_for_current_popup() -> None:
            expected_open_time = float(getattr(combo, "_popup_open_time_s", 0.0))

            def _force_hide() -> None:
                # If the popup has been reopened since we scheduled this hide,
                # ignore the delayed call.
                if float(getattr(combo, "_popup_open_time_s", 0.0)) != expected_open_time:
                    return

                try:
                    combo.hidePopup()
                except Exception:
                    pass

                # Extra defensive: hide/close the popup container window.
                try:
                    w = combo.view().window()  # QComboBoxPrivateContainer
                except Exception:
                    w = None

                if w is not None:
                    try:
                        w.hide()
                    except Exception:
                        pass
                    # Some backends only dismiss on Escape.
                    try:
                        from PySide6 import QtGui

                        press = QtGui.QKeyEvent(
                            QtCore.QEvent.Type.KeyPress,
                            QtCore.Qt.Key.Key_Escape,
                            QtCore.Qt.KeyboardModifier.NoModifier,
                        )
                        release = QtGui.QKeyEvent(
                            QtCore.QEvent.Type.KeyRelease,
                            QtCore.Qt.Key.Key_Escape,
                            QtCore.Qt.KeyboardModifier.NoModifier,
                        )
                        QtWidgets.QApplication.postEvent(w, press)
                        QtWidgets.QApplication.postEvent(w, release)
                    except Exception:
                        pass

            # Defer so Qt commits selection first, and retry a couple times.
            QtCore.QTimer.singleShot(0, _force_hide)
            QtCore.QTimer.singleShot(50, _force_hide)
            QtCore.QTimer.singleShot(150, _force_hide)

        # Close after user selection gestures.
        combo.activated.connect(lambda _idx: _schedule_hide_for_current_popup())

        # Also close when the popup view reports activation/click.
        try:
            view = combo.view()

            def _on_popup_index_triggered(model_index: QtCore.QModelIndex) -> None:
                if not model_index.isValid():
                    return

                # Ignore the spurious click delivered immediately after opening.
                open_time_s = float(getattr(combo, "_popup_open_time_s", 0.0))
                if open_time_s and (time.monotonic() - open_time_s) < 0.12:
                    if int(model_index.row()) == int(combo.currentIndex()):
                        return

                row = int(model_index.row())
                if combo.currentIndex() != row:
                    combo.setCurrentIndex(row)

                _schedule_hide_for_current_popup()

            view.clicked.connect(_on_popup_index_triggered)
            view.activated.connect(_on_popup_index_triggered)

            # Help repaint highlight during keyboard nav.
            try:
                view.selectionModel().currentChanged.connect(lambda *_: view.viewport().update())
            except Exception:
                pass
        except Exception:
            pass

    # --- widget factories ---

    def _add_bool(self, form: QFormLayout, label: str, target: object, attr: str) -> QCheckBox:
        # Put the checkbox *before* its label.
        cb = QCheckBox(form.parentWidget())
        cb.setChecked(bool(getattr(target, attr)))

        row = QWidget(form.parentWidget())
        row_layout = QtWidgets.QHBoxLayout(row)  # type: ignore
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(8)
        row_layout.addWidget(cb)
        lbl = QtWidgets.QLabel(label, row)
        # If the label contains a '&' mnemonic marker, set the checkbox as
        # the buddy so that Alt+<letter> toggles the checkbox.
        if "&" in label:
            lbl.setBuddy(cb)
            # On macOS, Alt-based mnemonics don't work; install a Ctrl/⌘
            # shortcut as a fallback.
            fix_label_buddy_for_mac(label, cb, self)
        row_layout.addWidget(lbl)
        row_layout.addStretch(1)
        form.addRow(row)

        self._bindings.append(
            _Binding(
                target=target,
                attr=attr,
                widget=cb,
                read=lambda w: bool(w.isChecked()),  # type: ignore[return-value]
                write=lambda w, v: w.setChecked(bool(v)),  # type: ignore[arg-type]
            )
        )
        return cb

    def _add_int(
        self,
        form: QFormLayout,
        label: str,
        target: object,
        attr: str,
        *,
        minimum: int = 0,
        maximum: int = 1_000_000,
    ) -> QSpinBox:
        sb = QSpinBox(form.parentWidget())
        sb.setRange(int(minimum), int(maximum))
        sb.setValue(int(getattr(target, attr)))
        form.addRow(label, sb)

        self._bindings.append(
            _Binding(
                target=target,
                attr=attr,
                widget=sb,
                read=lambda w: int(w.value()),  # type: ignore[return-value]
                write=lambda w, v: w.setValue(int(v)),  # type: ignore[arg-type]
            )
        )
        return sb

    def _add_str(self, form: QFormLayout, label: str, target: object, attr: str) -> QLineEdit:
        le = QLineEdit(form.parentWidget())
        le.setText(str(getattr(target, attr)))
        form.addRow(label, le)

        self._bindings.append(
            _Binding(
                target=target,
                attr=attr,
                widget=le,
                read=lambda w: str(w.text()),  # type: ignore[return-value]
                write=lambda w, v: w.setText(str(v)),  # type: ignore[arg-type]
            )
        )
        return le

    def _add_enum(self, form: QFormLayout, label: str, target: object, attr: str, enum_cls: type) -> QComboBox:
        combo = QComboBox(form.parentWidget())
        current = getattr(target, attr)
        # Track popup-open time to make delayed hidePopup calls safe.
        combo._popup_open_time_s = 0.0  # type: ignore[attr-defined]

        _orig_show_popup = combo.showPopup

        def _show_popup() -> None:
            combo._popup_open_time_s = time.monotonic()  # type: ignore[attr-defined]
            _orig_show_popup()

        combo.showPopup = _show_popup  # type: ignore[method-assign]

        self._close_combo_popup_on_selection(combo)
        # Keep stable ordering by Enum definition order.
        for member in enum_cls:  # type: ignore[assignment]
            combo.addItem(_enum_label(member.name), member)

        # set current
        idx = combo.findData(current)
        if idx >= 0:
            combo.setCurrentIndex(idx)
        form.addRow(label, combo)

        self._bindings.append(
            _Binding(
                target=target,
                attr=attr,
                widget=combo,
                read=lambda w: w.currentData(),  # type: ignore[return-value]
                write=lambda w, v: w.setCurrentIndex(w.findData(v)),  # type: ignore[arg-type]
            )
        )
        return combo

    def _sync_parent_geometry_to_config(self) -> None:
        """Copy the main window's current geometry into config so save won't overwrite it."""
        try:
            pw = self.parentWidget()
            if pw is None:
                return
            gui = self._config.gui
            if not gui.remember_position_and_size:
                return
            if pw.isMaximized():
                gui.start_maximized = True
            else:
                gui.start_maximized = False
                g = pw.geometry()
                gui.window_position_x = g.x()
                gui.window_position_y = g.y()
                gui.window_size_width = g.width()
                gui.window_size_height = g.height()
        except Exception:
            pass

    def _apply_binding(self, b: _Binding[Any]) -> None:
        """Apply a single widget value into config and persist it."""

        old_value = getattr(b.target, b.attr, None)
        new_value = b.read(b.widget)
        setattr(b.target, b.attr, new_value)
        log.debug("Setting changed: %s.%s = %r (was %r)", type(b.target).__name__, b.attr, new_value, old_value)

        # Before persisting, update the GUI position/size from the actual main
        # window so that save_settings() does not overwrite the current geometry
        # with stale values captured at startup.
        self._sync_parent_geometry_to_config()

        self._config.save_settings()

        # Apply theme changes immediately when the theme setting changes.
        if b.target is self._config.app and b.attr == "theme_mode":
            try:
                mode = self._config.app.theme_mode
                log.debug("Theme changed to: %s", mode.name)
                if mode == ThemeMode.DARK:
                    set_theme(app=QtWidgets.QApplication.instance(), mode="dark")  # type: ignore[arg-type]
                elif mode == ThemeMode.LIGHT:
                    set_theme(app=QtWidgets.QApplication.instance(), mode="light")  # type: ignore[arg-type]
                else:
                    set_theme(app=QtWidgets.QApplication.instance(), mode="system")  # type: ignore[arg-type]
            except Exception:
                # Theme switching is best-effort; don't break Settings.
                pass

        # Apply logging changes immediately when logging settings change.
        if b.target is self._config.logging and b.attr in ("enabled", "level"):
            self._apply_logging_config()

    def _apply_logging_config(self) -> None:
        """Reconfigure the logging subsystem from current config values."""
        try:
            level = int(getattr(self._config.logging, "level").value)
            enabled = bool(getattr(self._config.logging, "enabled"))
            log_dir = str(getattr(self._config.logging, "log_dir", ""))
            log_file = resolve_log_file(log_dir)
            configure_logging(enabled=enabled, level=level, log_file=log_file)
            log.info(
                "Logging reconfigured: enabled=%s level=%s path=%s",
                enabled, logging.getLevelName(level), str(log_file),
            )
        except Exception:
            pass

    def _wire_apply_on_change(self) -> None:
        """Connect widget change signals so settings persist immediately."""

        for b in self._bindings:
            w = b.widget
            if isinstance(w, QCheckBox):
                w.toggled.connect(lambda _checked, b=b: self._apply_binding(b))
            elif isinstance(w, QSpinBox):
                w.valueChanged.connect(lambda _v, b=b: self._apply_binding(b))
            elif isinstance(w, QLineEdit):
                w.textChanged.connect(lambda _t, b=b: self._apply_binding(b))
            elif isinstance(w, QComboBox):
                # currentIndexChanged ensures keyboard navigation also applies.
                w.currentIndexChanged.connect(lambda _i, b=b: self._apply_binding(b))
            else:
                # Fallback: no known signal.
                pass

    # --- QDialog hooks ---

    def accept(self) -> None:
        # No-op: settings are applied immediately.
        super().accept()
