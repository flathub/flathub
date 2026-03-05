from __future__ import annotations

"""Main application window.
"""

import os
import time
from pathlib import Path

from filename_ninja.logging_config import get_logger

log = get_logger("main_window")

from PySide6 import QtCore, QtWidgets
from PySide6.QtCore import QDir, QSize, Qt, QItemSelectionModel, QSortFilterProxyModel
from PySide6.QtGui import QIcon, QKeySequence, QUndoStack

from filename_ninja.ui.icon_utils import load_icon
from filename_ninja.ui.shortcut_helper import fix_label_buddy_for_mac, fix_mnemonic_for_mac, fix_msgbox_buttons_for_mac, shortcut_key, shortcut_tooltip
from PySide6.QtWidgets import (
    QAbstractItemView,
    QCheckBox,
    QDialog,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMessageBox,
    QProgressBar,
    QPushButton,
    QSizePolicy,
    QTabWidget,
    QTableView,
    QTreeView,
    QWidget,
    QFileSystemModel,
)

from ...config import AutoNumberChange, CaseChange, MainConfig, TextTransformConfig
from ...file_info import FileInfo
from ...models.rename_table_model import RenameTableModel
from ...resources import resources_rc  # noqa: F401
from ...services.folder_loader import FolderLoadController, LoadRequest
from ...services.scanner import ScanOptions
from ..about_dialog import AboutDialog
from ..help_dialog import HelpDialog
from ..regex_editor import RegexTextEdit
from ..settings_dialog import SettingsDialog

from .regex_reference import build_regex_reference_dock, toggle_regex_reference_dock
from .rename_ops import RenameCommand, RenameOp, rename_path
from .styles import (
    apply_compact_combo_box_style,
    apply_compact_line_edit_style,
    apply_compact_regex_text_edit_style,
    apply_compact_table_style,
    apply_uniform_row_height,
    compact_widget_height,
)
from .widgets import AutoCloseComboBox, HistoryLineEdit

# Minimum width for QMessageBox dialogs so that titles like
# "Rename", "Confirm rename", etc. are fully visible.
_MSG_BOX_MIN_WIDTH = 250


def _make_msg_box(
    icon: QMessageBox.Icon,
    parent: QtWidgets.QWidget | None,
    title: str,
    text: str,
    buttons: QMessageBox.StandardButton = QMessageBox.StandardButton.Ok,
) -> QMessageBox:
    """Create a QMessageBox with a guaranteed minimum width."""
    box = QMessageBox(icon, title, text, buttons, parent)
    box.setMinimumWidth(_MSG_BOX_MIN_WIDTH)
    # QMessageBox uses a QGridLayout internally; force the text column wider
    # so the dialog actually respects the minimum width.
    grid = box.layout()
    if isinstance(grid, QGridLayout):
        grid.setColumnMinimumWidth(grid.columnCount() - 1, _MSG_BOX_MIN_WIDTH)
    # On macOS, standard-button mnemonics (&Yes, &No, &Ok, …) don't work
    # because Alt-based mnemonics are not activated.  Install ⌃+<letter>
    # shortcuts as a fallback.
    fix_msgbox_buttons_for_mac(box)
    return box


class FilenameNinjaApp(QtWidgets.QMainWindow):
    def __init__(self, main_config: MainConfig | None = None):
        super().__init__()

        # Timers used by signal handlers may fire during UI construction.
        # Initialize them early so slots like `_on_rules_changed()` are always safe.
        self._rules_preview_timer = QtCore.QTimer(self)
        self._rules_preview_timer.setSingleShot(True)
        self._rules_preview_timer.timeout.connect(self._recalculate_preview_for_current_items)

        # Loading configuration stored in file
        self.main_config = main_config or MainConfig()

        # Undo support for filesystem renames.
        self._undo_stack: QUndoStack = QUndoStack(self)

        # Navigation history (Back/Forward).
        self._nav_back_stack: list[str] = []
        self._nav_forward_stack: list[str] = []

        app_icon = QIcon(":/icons/filename_ninja.png")
        self.setWindowIcon(app_icon)
        self.setWindowTitle("Filename Ninja")
        self.setGeometry(
            self.main_config.gui.window_position_x,
            self.main_config.gui.window_position_y,
            self.main_config.gui.window_size_width,
            self.main_config.gui.window_size_height,
        )
        self.central_widget = QWidget(self)
        self.setCentralWidget(self.central_widget)

        # Main layout
        self.main_layout = QtWidgets.QVBoxLayout()  # type: ignore

        self._rules_row_labels_by_index: dict[int, list[QLabel]] = {}
        self._rules_row_label_max_width: dict[int, int] = {}
        self._uncheck_after_rename_checkboxes: list[QCheckBox] = []

        # Build UI sections
        self._build_button_bar()
        self._build_navigation_bar()
        self._build_filesystem_views()
        self._build_rules_tabs()

        # Regex reference dock (extracted)
        self._regex_reference_dock = build_regex_reference_dock(self)

        self._apply_persisted_splitter_state()
        self._apply_persisted_dock_layout()

        # Background loading controller (Qt-aware service)
        self._loader = FolderLoadController(self)
        self._active_job_id = None
        self._busy_cursor_active = False

        self._file_mask_reload_timer = QtCore.QTimer(self)
        self._file_mask_reload_timer.setSingleShot(True)
        self._file_mask_reload_timer.timeout.connect(self._reload_for_current_file_mask)

        self._proposed_columns_resize_timer = QtCore.QTimer(self)
        self._proposed_columns_resize_timer.setSingleShot(True)
        self._proposed_columns_resize_timer.timeout.connect(self._resize_proposed_columns_to_contents)

        self._progress_timer = QtCore.QTimer(self)
        self._progress_timer.setSingleShot(True)
        self._progress_dialog: QDialog | None = None

        self._wire_loader_signals()
        self._wire_signals()

        self._undo_stack.canUndoChanged.connect(lambda can: self.undo_button.setEnabled(bool(can)))
        self._wire_table_sort_persistence()
        self._apply_persisted_table_sorting()
        self._sync_regex_rows_visibility()

        self.central_widget.setLayout(self.main_layout)  # type: ignore

    # ---- persisted layout ----

    def _apply_persisted_splitter_state(self) -> None:
        try:
            state = getattr(self.main_config.gui, "splitter_state", None)
            if state:
                ok = self.tree_folder_splitter.restoreState(state)
                if ok:
                    return
        except Exception:
            log.debug("Failed to restore persisted splitter state", exc_info=True)

    def _apply_persisted_dock_layout(self) -> None:
        try:
            state = getattr(self.main_config.gui, "main_window_state", None)
            if state:
                self.restoreState(state)
        except Exception:
            log.debug("Failed to restore persisted dock layout", exc_info=True)

        try:
            self.tree_folder_splitter.setStretchFactor(0, 1)
            self.tree_folder_splitter.setStretchFactor(1, 3)
            self.tree_folder_splitter.setStretchFactor(2, 3)
        except Exception:
            log.debug("Failed to set splitter stretch factors", exc_info=True)

    # ---- UI builders (still methods for now) ----

    def _build_button_bar(self) -> None:
        self.button_bar_layout = QHBoxLayout()

        self.settings_button = QPushButton()
        self.settings_button.setToolTip(shortcut_tooltip("Open settings", "Q"))
        self.settings_button.setShortcut(shortcut_key("Q"))
        self.settings_button.setText("")
        settings_icon = load_icon(
            "settings", "preferences-system",
            standard_pixmap=QtWidgets.QStyle.StandardPixmap.SP_FileDialogDetailedView,
            widget=self,
        )
        if not settings_icon.isNull():
            self.settings_button.setIcon(settings_icon)
            self.settings_button.setIconSize(QSize(16, 16))
        else:
            self.settings_button.setText("Settings")
        self.button_bar_layout.addWidget(self.settings_button)

        self.show_folders_button = QPushButton("Show &folders")
        self.show_folders_button.setCheckable(True)
        self.show_folders_button.setChecked(bool(self.main_config.app.show_folders))
        fix_mnemonic_for_mac(self.show_folders_button)
        self.button_bar_layout.addWidget(self.show_folders_button)

        self.show_files_button = QPushButton("Sho&w files")
        self.show_files_button.setCheckable(True)
        self.show_files_button.setChecked(bool(self.main_config.app.show_files))
        fix_mnemonic_for_mac(self.show_files_button)
        self.button_bar_layout.addWidget(self.show_files_button)

        self.load_subfolders_button = QPushButton("Include &subfolders")
        self.load_subfolders_button.setCheckable(True)
        self.load_subfolders_button.setChecked(bool(self.main_config.app.load_subfolders))
        fix_mnemonic_for_mac(self.load_subfolders_button)
        self.button_bar_layout.addWidget(self.load_subfolders_button)

        self.button_bar_layout.addStretch(1)

        self.regex_reference_button = QPushButton("Rege&x reference")
        self.regex_reference_button.setCheckable(True)
        self.regex_reference_button.setChecked(False)
        self.regex_reference_button.setVisible(False)
        fix_mnemonic_for_mac(self.regex_reference_button)
        self.button_bar_layout.addWidget(self.regex_reference_button)

        self.help_button = QPushButton()
        self.help_button.setToolTip("Help")
        self.help_button.setText("")
        help_icon = load_icon(
            "help-contents", "help-browser",
            standard_pixmap=QtWidgets.QStyle.StandardPixmap.SP_DialogHelpButton,
            widget=self,
        )
        if not help_icon.isNull():
            self.help_button.setIcon(help_icon)
            self.help_button.setIconSize(QSize(16, 16))
        else:
            self.help_button.setText("Help")
        self.button_bar_layout.addWidget(self.help_button)

        self.about_button = QPushButton()
        self.about_button.setToolTip("About")
        self.about_button.setText("")
        about_icon = load_icon(
            "help-about", "help",
            standard_pixmap=QtWidgets.QStyle.StandardPixmap.SP_MessageBoxInformation,
            widget=self,
        )
        if not about_icon.isNull():
            self.about_button.setIcon(about_icon)
            self.about_button.setIconSize(QSize(16, 16))
        else:
            self.about_button.setText("About")
        self.button_bar_layout.addWidget(self.about_button)

        self.main_layout.addLayout(self.button_bar_layout)  # type: ignore

    def _build_navigation_bar(self) -> None:
        self.navigation_layout = QHBoxLayout()

        self.back_button = QPushButton()
        self.back_button.setToolTip(shortcut_tooltip("Back", "Z"))
        self.back_button.setShortcut(shortcut_key("Z"))
        self.back_button.setEnabled(False)
        back_icon = load_icon(
            "go-previous",
            standard_pixmap=QtWidgets.QStyle.StandardPixmap.SP_ArrowBack,
            widget=self,
        )
        if not back_icon.isNull():
            self.back_button.setIcon(back_icon)
        else:
            self.back_button.setText("Back")
        self.navigation_layout.addWidget(self.back_button)

        self.forward_button = QPushButton()
        self.forward_button.setToolTip(shortcut_tooltip("Forward", "Y"))
        self.forward_button.setShortcut(shortcut_key("Y"))
        self.forward_button.setEnabled(False)
        forward_icon = load_icon(
            "go-next",
            standard_pixmap=QtWidgets.QStyle.StandardPixmap.SP_ArrowForward,
            widget=self,
        )
        if not forward_icon.isNull():
            self.forward_button.setIcon(forward_icon)
        else:
            self.forward_button.setText("Forward")
        self.navigation_layout.addWidget(self.forward_button)

        self.parent_dir_button = QPushButton()
        parent_icon = load_icon(
            "go-up", "folder-open",
            standard_pixmap=QtWidgets.QStyle.StandardPixmap.SP_FileDialogToParent,
            widget=self,
        )
        self.parent_dir_button.setIcon(parent_icon)
        self.parent_dir_button.setIconSize(parent_icon.actualSize(QSize(16, 16)))
        self.parent_dir_button.setToolTip(shortcut_tooltip("Go to parent folder", "J"))
        self.parent_dir_button.setShortcut(shortcut_key("J"))
        self.navigation_layout.addWidget(self.parent_dir_button)

        self.current_path = HistoryLineEdit()
        self.current_path.setPlaceholderText("Enter folder path")
        self.current_path.setText(self.main_config.app.current_path)
        self.current_path.set_history(self.main_config.app.recent_paths)
        apply_compact_line_edit_style(self.current_path)
        self.current_q_dir = QDir(self.current_path.text())
        self.navigation_layout.addWidget(self.current_path, 3)

        self.file_mask = HistoryLineEdit()
        self.file_mask.setPlaceholderText("*.*")
        self.file_mask.setText(self.main_config.app.file_mask)
        self.file_mask.set_history(self.main_config.app.recent_masks)
        apply_compact_line_edit_style(self.file_mask)
        self.navigation_layout.addWidget(self.file_mask, 1)

        self.main_layout.addLayout(self.navigation_layout)  # type: ignore

    # ---- copied utility methods for rule widgets (still in this class) ----

    def _enum_label(self, name: str) -> str:
        return name.replace("_", " ").title()

    def _close_combo_popup_on_selection(self, combo) -> None:
        # kept identical to original for now
        def _schedule_hide_for_current_popup() -> None:
            expected_seq = int(getattr(combo, "_popup_open_seq", 0))

            def _force_hide() -> None:
                if int(getattr(combo, "_popup_open_seq", 0)) != expected_seq:
                    return
                try:
                    combo.hidePopup()
                except Exception:
                    log.debug("Failed to hide combo popup", exc_info=True)
                try:
                    w = combo.view().window()
                except Exception:
                    log.debug("Failed to get combo popup window", exc_info=True)
                    w = None
                if w is not None:
                    try:
                        w.hide()
                    except Exception:
                        log.debug("Failed to hide combo popup window", exc_info=True)
                    try:
                        from PySide6 import QtGui

                        press = QtGui.QKeyEvent(QtCore.QEvent.Type.KeyPress, Qt.Key.Key_Escape, Qt.KeyboardModifier.NoModifier)
                        release = QtGui.QKeyEvent(
                            QtCore.QEvent.Type.KeyRelease, Qt.Key.Key_Escape, Qt.KeyboardModifier.NoModifier
                        )
                        QtWidgets.QApplication.postEvent(w, press)
                        QtWidgets.QApplication.postEvent(w, release)
                    except Exception:
                        log.debug("Failed to send escape key to combo popup", exc_info=True)

            QtCore.QTimer.singleShot(0, _force_hide)
            QtCore.QTimer.singleShot(50, _force_hide)
            QtCore.QTimer.singleShot(150, _force_hide)

        combo.activated.connect(lambda _idx: _schedule_hide_for_current_popup())

        try:
            view = combo.view()

            def _on_popup_index_triggered(model_index: QtCore.QModelIndex) -> None:
                if not model_index.isValid():
                    return
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
            try:
                view.selectionModel().currentChanged.connect(lambda *_: view.viewport().update())
            except Exception:
                log.debug("Failed to connect selection model currentChanged", exc_info=True)
        except Exception:
            log.debug("Failed to set up combo popup click handlers", exc_info=True)

    def _make_enum_widget(self, parent: QWidget, target: object, attr: str, enum_cls: type):
        combo = AutoCloseComboBox(parent)
        current = getattr(target, attr)
        for member in enum_cls:  # type: ignore[assignment]
            combo.addItem(self._enum_label(member.name), member)
        idx = combo.findData(current)
        if idx >= 0:
            combo.setCurrentIndex(idx)

        def on_changed(_idx: int) -> None:
            data = combo.currentData()
            if data is not None:
                setattr(target, attr, data)
            self._on_rules_changed()

        combo.currentIndexChanged.connect(on_changed)
        self._close_combo_popup_on_selection(combo)
        apply_compact_combo_box_style(combo)
        return combo

    def _make_str_widget(
        self,
        parent: QWidget,
        target: object,
        attr: str,
        *,
        history_attr: str | None = None,
    ) -> QLineEdit:
        if history_attr is not None and isinstance(target, TextTransformConfig):
            le = HistoryLineEdit(parent)
            le.setText(str(getattr(target, attr)))
            le.set_history(getattr(target, history_attr, []))
            # Store references so _refresh_text_field_histories can find matching editors.
            le._history_target = target  # type: ignore[attr-defined]
            le._history_attr = history_attr  # type: ignore[attr-defined]
            le.historyItemRemoved.connect(
                lambda val, t=target, ha=history_attr: self._on_text_field_history_removed(t, ha, val)
            )
            le.returnPressed.connect(
                lambda ed=le, t=target, ha=history_attr: self._save_text_field_history(ed, t, ha)
            )
        else:
            le = QLineEdit(parent)
            le.setText(str(getattr(target, attr)))
        apply_compact_line_edit_style(le)
        le.textChanged.connect(lambda v: setattr(target, attr, str(v)))
        le.textChanged.connect(lambda _v: self._on_rules_changed())
        return le

    def _make_regex_widget(
        self,
        parent: QWidget,
        target: object,
        attr: str,
        *,
        kind: str,
        history: list[str] | None = None,
    ) -> RegexTextEdit:
        te = RegexTextEdit(parent, kind=kind)
        te.setPlainText(str(getattr(target, attr)))
        if history is not None:
            te.set_history(history)
        apply_compact_regex_text_edit_style(te)
        te.textChangedSingleLine.connect(lambda v: setattr(target, attr, str(v)))
        te.textChangedSingleLine.connect(lambda _v: self._on_rules_changed())
        return te

    def _make_int_widget(self, parent: QWidget, target: object, attr: str, *, minimum: int = 0, maximum: int = 1_000_000):
        sb = QtWidgets.QSpinBox(parent)
        sb.setRange(int(minimum), int(maximum))
        sb.setValue(int(getattr(target, attr)))
        sb.valueChanged.connect(lambda v: setattr(target, attr, int(v)))
        sb.valueChanged.connect(lambda _v: self._on_rules_changed())
        try:
            sb.setFixedHeight(compact_widget_height())
        except Exception:
            log.debug("Failed to set spin box fixed height", exc_info=True)
        sb.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        return sb

    def _add_active_group_row(self, form: QFormLayout, target: object, active_attr: str, parts, tab_order_widgets: list[QWidget] | None = None):
        cb = QCheckBox(form.parentWidget())
        cb.setChecked(bool(getattr(target, active_attr)))
        cb.toggled.connect(lambda v: setattr(target, active_attr, bool(v)))
        cb.toggled.connect(lambda _v: self._on_rules_changed())

        row = QWidget(form.parentWidget())
        row_layout = QHBoxLayout(row)  # type: ignore
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(8)
        row_layout.addWidget(cb)

        widgets_to_toggle: list[QWidget] = []
        first_label: QLabel | None = None
        for idx, (label, w) in enumerate(parts):
            lab = QLabel(label, row)
            if first_label is None:
                first_label = lab
            lab.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Preferred)
            row_layout.addWidget(lab)
            row_layout.addWidget(w)
            widgets_to_toggle.append(w)
            labels = self._rules_row_labels_by_index.setdefault(idx, [])
            labels.append(lab)
            max_w = self._rules_row_label_max_width.get(idx, 0)
            hinted = int(lab.sizeHint().width())
            if hinted > max_w:
                max_w = hinted
                self._rules_row_label_max_width[idx] = max_w
                for existing in labels:
                    existing.setMinimumWidth(max_w)
            else:
                lab.setMinimumWidth(max_w)
            if isinstance(w, (QLineEdit, QtWidgets.QSpinBox, QtWidgets.QComboBox, RegexTextEdit)):
                row_layout.setStretchFactor(w, 1)

        # Apply uniform height *after* all children have been added so that
        # apply_uniform_row_height can detect RegexTextEdit widgets and use
        # their natural height instead of clipping them.
        apply_uniform_row_height(row)

        enabled = bool(cb.isChecked())
        for w in widgets_to_toggle:
            w.setEnabled(enabled)
            cb.toggled.connect(w.setEnabled)

        if first_label is not None:
            first_label.setBuddy(cb)
            # On macOS, Alt-based label mnemonics don't work; install a
            # ⌃ (Control) shortcut as a fallback.  Parent the shortcut to
            # *row* (not *self*) so that duplicate mnemonic letters across
            # tabs (Name vs Extension) don't cause ambiguity — Qt skips
            # shortcuts whose parent widget is hidden.
            if parts:
                fix_label_buddy_for_mac(parts[0][0], cb, row)

        # Collect focusable widgets in visual order for explicit tab-order setup.
        if tab_order_widgets is not None:
            tab_order_widgets.append(cb)
            tab_order_widgets.extend(widgets_to_toggle)

        form.addRow(row)
        return cb

    def _add_sanitize_row(self, form: QFormLayout, target: object, tab_order_widgets: list[QWidget] | None = None):
        row = QWidget(form.parentWidget())
        row_layout = QHBoxLayout(row)  # type: ignore
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(8)
        apply_uniform_row_height(row)

        cb_acc = QCheckBox(row)
        cb_acc.setChecked(bool(getattr(target, "remove_accents")))
        cb_acc.toggled.connect(lambda v: setattr(target, "remove_accents", bool(v)))
        cb_acc.toggled.connect(lambda _v: self._on_rules_changed())
        row_layout.addWidget(cb_acc)
        lbl_acc = QLabel("Re&move accents", row)
        lbl_acc.setBuddy(cb_acc)
        fix_label_buddy_for_mac("Re&move accents", cb_acc, row)
        lbl_acc.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Preferred)
        row_layout.addWidget(lbl_acc)

        # Register lbl_acc in the label-width tracking system (index 0)
        # so it shares the same minimum width as the first labels in other
        # rows created by _add_active_group_row.  This pushes the
        # "Remove non-alphanumeric" checkbox to align with the first
        # QLineEdit / combo / spinbox on neighbouring rows.
        labels_0 = self._rules_row_labels_by_index.setdefault(0, [])
        labels_0.append(lbl_acc)
        max_w = self._rules_row_label_max_width.get(0, 0)
        hinted = int(lbl_acc.sizeHint().width())
        if hinted > max_w:
            max_w = hinted
            self._rules_row_label_max_width[0] = max_w
            for existing in labels_0:
                existing.setMinimumWidth(max_w)
        else:
            lbl_acc.setMinimumWidth(max_w)

        cb_non = QCheckBox(row)
        cb_non.setChecked(bool(getattr(target, "remove_non_alphanumeric")))
        cb_non.toggled.connect(lambda v: setattr(target, "remove_non_alphanumeric", bool(v)))
        cb_non.toggled.connect(lambda _v: self._on_rules_changed())
        row_layout.addWidget(cb_non)
        lbl_non = QLabel("Remo&ve non-alphanumeric characters", row)
        lbl_non.setBuddy(cb_non)
        fix_label_buddy_for_mac("Remo&ve non-alphanumeric characters", cb_non, row)
        row_layout.addWidget(lbl_non)

        row_layout.addWidget(QLabel("Keep these", row))
        keep_le = HistoryLineEdit(row)
        keep_le.setText(str(getattr(target, "keep_these_non_alphanumeric")))
        keep_le.setPlaceholderText("Non-alphanumeric characters to keep")
        keep_le.set_history(self.main_config.app.recent_keep_these)
        keep_le.historyItemRemoved.connect(self._on_keep_these_history_removed)
        keep_le.textChanged.connect(lambda v: setattr(target, "keep_these_non_alphanumeric", str(v)))
        keep_le.textChanged.connect(lambda _v: self._on_rules_changed())
        keep_le.returnPressed.connect(lambda: self._save_keep_these_history(keep_le))
        keep_le.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        row_layout.addWidget(keep_le, 1)
        keep_le.setEnabled(bool(cb_non.isChecked()))
        cb_non.toggled.connect(keep_le.setEnabled)

        # Collect focusable widgets in visual order for explicit tab-order setup.
        if tab_order_widgets is not None:
            tab_order_widgets.extend([cb_acc, cb_non, keep_le])

        form.addRow(row)
        return cb_acc, cb_non

    # ---- rules tabs and filesystem views ----

    def _build_rules_tabs(self) -> None:
        self.rules_tabs = QTabWidget(self.central_widget)
        self.rules_tabs.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Maximum)

        # Collect focusable widgets in visual order per tab so we can set an
        # explicit tab-order chain after all rows have been created.
        name_tab_order: list[QWidget] = []
        ext_tab_order: list[QWidget] = []

        name_tab = QWidget(self.rules_tabs)
        name_form = QFormLayout(name_tab)  # type: ignore
        self.rules_tabs.addTab(name_tab, "Name")

        cb_acc, cb_non = self._add_sanitize_row(name_form, self.main_config.name, tab_order_widgets=name_tab_order)
        self._uncheck_after_rename_checkboxes.extend([cb_acc, cb_non])

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                name_form,
                self.main_config.name,
                "is_active_text_to_replace",
                [
                    ("&Text to replace", self._make_str_widget(name_form.parentWidget(), self.main_config.name, "text_to_replace", history_attr="recent_text_to_replace")),
                    (
                        "Text for replacing",
                        self._make_str_widget(name_form.parentWidget(), self.main_config.name, "text_for_replacing", history_attr="recent_text_for_replacing"),
                    ),
                ],
                tab_order_widgets=name_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                name_form,
                self.main_config.name,
                "is_active_left_crop_n_characters",
                [("&Left crop N", self._make_int_widget(name_form.parentWidget(), self.main_config.name, "left_crop_n_characters"))],
                tab_order_widgets=name_tab_order,
            )
        )
        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                name_form,
                self.main_config.name,
                "is_active_right_crop_n_characters",
                [("Ri&ght crop N", self._make_int_widget(name_form.parentWidget(), self.main_config.name, "right_crop_n_characters"))],
                tab_order_widgets=name_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                name_form,
                self.main_config.name,
                "is_active_crop_at_position",
                [
                    ("Cro&p at position", self._make_int_widget(name_form.parentWidget(), self.main_config.name, "crop_at_position")),
                    ("Crop how many", self._make_int_widget(name_form.parentWidget(), self.main_config.name, "crop_how_many")),
                ],
                tab_order_widgets=name_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                name_form,
                self.main_config.name,
                "is_active_insert_before",
                [("Insert &before", self._make_str_widget(name_form.parentWidget(), self.main_config.name, "insert_before", history_attr="recent_insert_before"))],
                tab_order_widgets=name_tab_order,
            )
        )
        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                name_form,
                self.main_config.name,
                "is_active_insert_after",
                [("&Insert after", self._make_str_widget(name_form.parentWidget(), self.main_config.name, "insert_after", history_attr="recent_insert_after"))],
                tab_order_widgets=name_tab_order,
            )
        )
        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                name_form,
                self.main_config.name,
                "is_active_insert_at_position",
                [
                    ("Insert at p&osition", self._make_int_widget(name_form.parentWidget(), self.main_config.name, "insert_at_position")),
                    ("Insert what", self._make_str_widget(name_form.parentWidget(), self.main_config.name, "insert_what", history_attr="recent_insert_what")),
                ],
                tab_order_widgets=name_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                name_form,
                self.main_config.name,
                "is_active_case_change",
                [("Case c&hange", self._make_enum_widget(name_form.parentWidget(), self.main_config.name, "case_change", CaseChange))],
                tab_order_widgets=name_tab_order,
            )
        )

        self._regex_pattern_edit = self._make_regex_widget(
            name_form.parentWidget(),
            self.main_config.regex,
            "regex_pattern",
            kind="pattern",
            history=self.main_config.regex.recent_regex_patterns,
        )
        self._regex_replacement_edit = self._make_regex_widget(
            name_form.parentWidget(),
            self.main_config.regex,
            "regex_replacement",
            kind="replacement",
            history=self.main_config.regex.recent_regex_replacements,
        )
        self._regex_name_row = self._add_active_group_row(
            name_form,
            self.main_config.regex,
            "is_active_regex_rename",
            [
                ("R&egex match", self._regex_pattern_edit),
                ("Regex replacement", self._regex_replacement_edit),
            ],
            tab_order_widgets=name_tab_order,
        )
        self._uncheck_after_rename_checkboxes.append(self._regex_name_row)

        ext_tab = QWidget(self.rules_tabs)
        ext_form = QFormLayout(ext_tab)  # type: ignore
        self.rules_tabs.addTab(ext_tab, "Extension")

        cb_acc, cb_non = self._add_sanitize_row(ext_form, self.main_config.ext, tab_order_widgets=ext_tab_order)
        self._uncheck_after_rename_checkboxes.extend([cb_acc, cb_non])

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                ext_form,
                self.main_config.ext,
                "is_active_text_to_replace",
                [
                    ("&Text to replace", self._make_str_widget(ext_form.parentWidget(), self.main_config.ext, "text_to_replace", history_attr="recent_text_to_replace")),
                    (
                        "Text for replacing",
                        self._make_str_widget(ext_form.parentWidget(), self.main_config.ext, "text_for_replacing", history_attr="recent_text_for_replacing"),
                    ),
                ],
                tab_order_widgets=ext_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                ext_form,
                self.main_config.ext,
                "is_active_left_crop_n_characters",
                [("&Left crop N", self._make_int_widget(ext_form.parentWidget(), self.main_config.ext, "left_crop_n_characters"))],
                tab_order_widgets=ext_tab_order,
            )
        )
        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                ext_form,
                self.main_config.ext,
                "is_active_right_crop_n_characters",
                [("Ri&ght crop N", self._make_int_widget(ext_form.parentWidget(), self.main_config.ext, "right_crop_n_characters"))],
                tab_order_widgets=ext_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                ext_form,
                self.main_config.ext,
                "is_active_crop_at_position",
                [
                    ("Cro&p at position", self._make_int_widget(ext_form.parentWidget(), self.main_config.ext, "crop_at_position")),
                    ("Crop how many", self._make_int_widget(ext_form.parentWidget(), self.main_config.ext, "crop_how_many")),
                ],
                tab_order_widgets=ext_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                ext_form,
                self.main_config.ext,
                "is_active_insert_before",
                [("Insert &before", self._make_str_widget(ext_form.parentWidget(), self.main_config.ext, "insert_before", history_attr="recent_insert_before"))],
                tab_order_widgets=ext_tab_order,
            )
        )
        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                ext_form,
                self.main_config.ext,
                "is_active_insert_after",
                [("&Insert after", self._make_str_widget(ext_form.parentWidget(), self.main_config.ext, "insert_after", history_attr="recent_insert_after"))],
                tab_order_widgets=ext_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                ext_form,
                self.main_config.ext,
                "is_active_insert_at_position",
                [
                    ("Insert at p&osition", self._make_int_widget(ext_form.parentWidget(), self.main_config.ext, "insert_at_position")),
                    ("Insert what", self._make_str_widget(ext_form.parentWidget(), self.main_config.ext, "insert_what", history_attr="recent_insert_what")),
                ],
                tab_order_widgets=ext_tab_order,
            )
        )

        self._uncheck_after_rename_checkboxes.append(
            self._add_active_group_row(
                ext_form,
                self.main_config.ext,
                "is_active_case_change",
                [("Case c&hange", self._make_enum_widget(ext_form.parentWidget(), self.main_config.ext, "case_change", CaseChange))],
                tab_order_widgets=ext_tab_order,
            )
        )

        # Set explicit tab order for each tab so that Tab moves focus through
        # fields in visual (top-to-bottom, left-to-right) order.  Qt
        # automatically skips disabled/hidden widgets at runtime.
        for tab_widgets in (name_tab_order, ext_tab_order):
            for i in range(len(tab_widgets) - 1):
                QWidget.setTabOrder(tab_widgets[i], tab_widgets[i + 1])

        self._sync_regex_rows_visibility()

        self.numbering_group = QGroupBox(self.central_widget)
        self.numbering_group.setTitle("")
        self.numbering_group.setFlat(True)
        self.numbering_group.setSizePolicy(QSizePolicy.Policy.Maximum, QSizePolicy.Policy.Maximum)
        numbering_root = QtWidgets.QVBoxLayout(self.numbering_group)  # type: ignore
        numbering_root.setContentsMargins(8, 8, 8, 8)

        num_active_row = QWidget(self.numbering_group)
        num_active_row_layout = QHBoxLayout(num_active_row)  # type: ignore
        num_active_row_layout.setContentsMargins(0, 0, 0, 0)
        num_active_row_layout.setSpacing(8)
        apply_uniform_row_height(num_active_row)

        num_active_cb = QCheckBox(num_active_row)
        num_active_cb.setChecked(bool(self.main_config.numbering.is_active_auto_number_change))
        num_active_cb.toggled.connect(
            lambda v: setattr(self.main_config.numbering, "is_active_auto_number_change", bool(v))
        )
        num_active_cb.toggled.connect(lambda _v: self._on_rules_changed())
        num_active_row_layout.addWidget(num_active_cb)
        self._uncheck_after_rename_checkboxes.append(num_active_cb)

        _auto_num_label = QLabel("&Automatic numbering", num_active_row)
        _auto_num_label.setBuddy(num_active_cb)
        fix_label_buddy_for_mac("&Automatic numbering", num_active_cb, self)
        num_active_row_layout.addWidget(_auto_num_label)
        num_change_combo = self._make_enum_widget(num_active_row, self.main_config.numbering, "auto_number_change", AutoNumberChange)
        num_change_combo.setSizePolicy(QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Fixed)
        num_active_row_layout.addWidget(num_change_combo, 1)
        numbering_root.addWidget(num_active_row)

        self.numbering_details = QWidget(self.numbering_group)
        num_form = QFormLayout(self.numbering_details)  # type: ignore
        numbering_root.addWidget(self.numbering_details)

        self._add_int(num_form, "Start with", self.main_config.numbering, "start_with", minimum=0, maximum=1_000_000)
        self._add_int(num_form, "Increment by", self.main_config.numbering, "increment_by", minimum=1, maximum=1_000)
        self._add_int(num_form, "Zero fill (digits)", self.main_config.numbering, "zero_fill_how_many", minimum=1, maximum=64)
        self._add_bool(num_form, "&New numbering for each folder", self.main_config.numbering, "new_numbering_for_each_folder")

        enabled = bool(num_active_cb.isChecked())
        num_change_combo.setEnabled(enabled)
        self.numbering_details.setEnabled(enabled)
        num_active_cb.toggled.connect(num_change_combo.setEnabled)
        num_active_cb.toggled.connect(self.numbering_details.setEnabled)

        self.rename_plan_info = QLabel("", self.central_widget)
        self.rename_plan_info.setWordWrap(False)
        self.rename_plan_info.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)
        # Default to normal text; we switch to a "disabled label" color only when
        # there are 0 planned renames.
        self.rename_plan_info.setStyleSheet("")
        self.rename_plan_info.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        self._update_rename_plan_info()

        self.rename_undo_group = QGroupBox(self.central_widget)
        self.rename_undo_group.setTitle("")
        self.rename_undo_group.setFlat(True)
        self.rename_undo_group.setSizePolicy(QSizePolicy.Policy.Maximum, QSizePolicy.Policy.Maximum)
        rename_undo_root = QtWidgets.QVBoxLayout(self.rename_undo_group)  # type: ignore
        rename_undo_root.setContentsMargins(8, 8, 8, 8)
        rename_undo_root.setSpacing(8)

        self.rename_folders_cb = QCheckBox("Rename fol&ders", self.rename_undo_group)
        self.rename_folders_cb.setChecked(bool(self.main_config.app.rename_folders))
        fix_mnemonic_for_mac(self.rename_folders_cb)

        def _on_rename_folders_toggled(v: bool) -> None:
            self.main_config.app.rename_folders = bool(v)
            self._reload_for_current_file_mask()

        self.rename_folders_cb.toggled.connect(_on_rename_folders_toggled)
        rename_undo_root.addWidget(self.rename_folders_cb)

        self.only_rename_selected_cb = QCheckBox("Only rename sele&cted files", self.rename_undo_group)
        self.only_rename_selected_cb.setChecked(bool(self.main_config.app.only_rename_selected_files))
        fix_mnemonic_for_mac(self.only_rename_selected_cb)

        def _on_only_selected_toggled(v: bool) -> None:
            self.main_config.app.only_rename_selected_files = bool(v)
            self._recalculate_preview_for_current_items()

        self.only_rename_selected_cb.toggled.connect(_on_only_selected_toggled)
        rename_undo_root.addWidget(self.only_rename_selected_cb)

        btn_row = QWidget(self.rename_undo_group)
        btn_row_layout = QHBoxLayout(btn_row)  # type: ignore
        btn_row_layout.setContentsMargins(0, 0, 0, 0)
        btn_row_layout.setSpacing(8)

        self.rename_button = QPushButton("&Rename", btn_row)
        fix_mnemonic_for_mac(self.rename_button)
        rename_icon = load_icon(
            "edit-rename", "document-save",
            standard_pixmap=QtWidgets.QStyle.StandardPixmap.SP_DialogApplyButton,
            widget=self,
        )
        if not rename_icon.isNull():
            self.rename_button.setIcon(rename_icon)
            self.rename_button.setIconSize(QSize(16, 16))

        self.undo_button = QPushButton("&Undo", btn_row)
        fix_mnemonic_for_mac(self.undo_button)
        self.undo_button.setEnabled(False)
        undo_icon = load_icon(
            "edit-undo",
            standard_pixmap=QtWidgets.QStyle.StandardPixmap.SP_ArrowBack,
            widget=self,
        )
        if not undo_icon.isNull():
            self.undo_button.setIcon(undo_icon)
            self.undo_button.setIconSize(QSize(16, 16))

        btn_row_layout.addWidget(self.rename_button)
        btn_row_layout.addWidget(self.undo_button)
        btn_row_layout.addStretch(1)
        rename_undo_root.addWidget(btn_row)

        self.rename_button.clicked.connect(self._on_rename_clicked)
        self.undo_button.clicked.connect(self._on_undo_clicked)

        self.rules_section = QWidget(self.central_widget)
        rules_section_layout = QHBoxLayout(self.rules_section)  # type: ignore
        rules_section_layout.setContentsMargins(0, 0, 0, 0)
        rules_section_layout.setSpacing(10)
        self.rules_tabs.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Maximum)
        rules_section_layout.addWidget(self.rules_tabs, 1)
        rules_section_layout.setAlignment(self.rules_tabs, Qt.AlignmentFlag.AlignTop)

        self.right_side_groupboxes = QWidget(self.rules_section)
        right_side_layout = QtWidgets.QVBoxLayout(self.right_side_groupboxes)  # type: ignore
        right_side_layout.setContentsMargins(0, 0, 0, 0)
        right_side_layout.setSpacing(8)
        right_side_layout.addWidget(self.rename_plan_info, 0)
        right_side_layout.addWidget(self.numbering_group, 0)
        right_side_layout.addWidget(self.rename_undo_group, 0)
        right_side_layout.addStretch(1)
        rules_section_layout.addWidget(self.right_side_groupboxes, 0)
        rules_section_layout.setAlignment(self.right_side_groupboxes, Qt.AlignmentFlag.AlignTop)

        QtCore.QTimer.singleShot(0, self._sync_right_groupbox_widths)
        self.main_layout.addWidget(self.rules_section, 0)
        self.rules_tabs.currentChanged.connect(lambda _i: self._update_rules_tabs_height())
        QtCore.QTimer.singleShot(0, self._update_rules_tabs_height)

    def _add_bool(self, form: QFormLayout, label: str, target: object, attr: str) -> QCheckBox:
        cb = QCheckBox(form.parentWidget())
        cb.setChecked(bool(getattr(target, attr)))
        row = QWidget(form.parentWidget())
        row_layout = QHBoxLayout(row)  # type: ignore
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(8)
        row_layout.addWidget(cb)
        lbl = QLabel(label, row)
        lbl.setBuddy(cb)
        if "&" in label:
            fix_label_buddy_for_mac(label, cb, self)
        row_layout.addWidget(lbl)
        row_layout.addStretch(1)
        form.addRow(row)
        cb.toggled.connect(lambda v: setattr(target, attr, bool(v)))
        cb.toggled.connect(lambda _v: self._on_rules_changed())
        return cb

    def _add_int(self, form: QFormLayout, label: str, target: object, attr: str, *, minimum: int = 0, maximum: int = 1_000_000):
        sb = QtWidgets.QSpinBox(form.parentWidget())
        sb.setRange(int(minimum), int(maximum))
        sb.setValue(int(getattr(target, attr)))
        try:
            sb.setFixedHeight(compact_widget_height())
        except Exception:
            log.debug("Failed to set spin box fixed height", exc_info=True)
        form.addRow(label, sb)
        sb.valueChanged.connect(lambda v: setattr(target, attr, int(v)))
        sb.valueChanged.connect(lambda _v: self._on_rules_changed())
        return sb

    def _build_filesystem_views(self) -> None:
        self.list_layout = QHBoxLayout()
        self.tree_folder_splitter = QtWidgets.QSplitter()
        # Use a proxy model to enforce sorting even on providers where the native
        # folder enumeration order is unstable (e.g. the Windows "\\wsl$" network
        # filesystem). Some Qt/platform combos ignore QTreeView sorting with a
        # QFileSystemModel directly; a QSortFilterProxyModel reliably applies the
        # sort.
        self.filesystem_model = QFileSystemModel()
        self.filesystem_model.setRootPath(QDir.rootPath())
        self.filesystem_model.setFilter(QDir.Dirs | QDir.NoDotAndDotDot)  # type: ignore

        self.filesystem_sort_proxy = QSortFilterProxyModel(self)
        self.filesystem_sort_proxy.setSourceModel(self.filesystem_model)
        self.filesystem_sort_proxy.setDynamicSortFilter(True)
        self.filesystem_sort_proxy.sort(0, Qt.SortOrder.AscendingOrder)
        self.tree_view = QTreeView()
        self.tree_view.setModel(self.filesystem_sort_proxy)
        self.tree_view.setSortingEnabled(True)
        # Ensure the initial sort order is A→Z (some styles/platforms default to descending).
        self.tree_view.sortByColumn(0, Qt.SortOrder.AscendingOrder)

        # Map the current path from the source model to the proxy model.
        current_index_model = self.filesystem_model.index(self.main_config.app.current_path)
        current_index_proxy = self.filesystem_sort_proxy.mapFromSource(current_index_model)
        self.tree_view.setExpanded(current_index_proxy, True)
        self.tree_view.setCurrentIndex(current_index_proxy)
        self.tree_view.setHeaderHidden(True)
        for column in range(1, self.filesystem_model.columnCount()):
            self.tree_view.setColumnHidden(column, True)
        self.tree_view.resizeColumnToContents(0)
        self.tree_folder_splitter.addWidget(self.tree_view)

        self.tableViewCurrent = QTableView()
        self.tableViewCurrent.setSortingEnabled(True)
        self.tableViewCurrent.verticalHeader().setVisible(False)
        self.tableViewCurrent.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.tableViewCurrent.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
        self.tableViewCurrent.setVerticalScrollMode(QAbstractItemView.ScrollMode.ScrollPerPixel)
        self.tableViewCurrent.setHorizontalScrollMode(QAbstractItemView.ScrollMode.ScrollPerPixel)
        self.rename_model = RenameTableModel([])
        self.tableViewCurrent.setModel(self.rename_model)
        apply_compact_table_style(self.tableViewCurrent)
        try:
            self.tableViewCurrent.horizontalHeader().setStretchLastSection(True)
        except Exception:
            log.debug("Failed to set stretch last section on current table", exc_info=True)
        self.tree_folder_splitter.addWidget(self.tableViewCurrent)

        self.tableViewRenamed = QTableView()
        self.tableViewRenamed.setSortingEnabled(True)
        self.tableViewRenamed.verticalHeader().setVisible(False)
        self.tableViewRenamed.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.tableViewRenamed.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
        self.tableViewRenamed.setVerticalScrollMode(QAbstractItemView.ScrollMode.ScrollPerPixel)
        self.tableViewRenamed.setHorizontalScrollMode(QAbstractItemView.ScrollMode.ScrollPerPixel)
        self.tableViewRenamed.setModel(self.rename_model)
        apply_compact_table_style(self.tableViewRenamed)
        try:
            self.tableViewRenamed.horizontalHeader().setStretchLastSection(True)
        except Exception:
            log.debug("Failed to set stretch last section on renamed table", exc_info=True)
        self.tree_folder_splitter.addWidget(self.tableViewRenamed)

        self.tableViewCurrent.setColumnHidden(RenameTableModel.NEW_NAME_COL, True)
        self.tableViewCurrent.setColumnHidden(RenameTableModel.NEW_EXT_COL, True)
        self.tableViewRenamed.setColumnHidden(RenameTableModel.ORIG_NAME_COL, True)
        self.tableViewRenamed.setColumnHidden(RenameTableModel.ORIG_EXT_COL, True)
        for col in (
            RenameTableModel.SIZE_COL,
            RenameTableModel.TYPE_COL,
            RenameTableModel.PERMS_COL,
            RenameTableModel.PATH_COL,
            RenameTableModel.CREATED_COL,
            RenameTableModel.MODIFIED_COL,
            RenameTableModel.ACCESSED_COL,
        ):
            self.tableViewRenamed.setColumnHidden(col, True)

        self._connect_table_scrollbars()
        self._connect_table_selection()

        self.tree_folder_splitter.setStretchFactor(0, 1)
        self.tree_folder_splitter.setStretchFactor(1, 3)
        self.tree_folder_splitter.setStretchFactor(2, 3)
        self.list_layout.addWidget(self.tree_folder_splitter)
        self.main_layout.addLayout(self.list_layout, 1)  # type: ignore
        try:
            self.tree_folder_splitter.splitterMoved.connect(self._on_splitter_moved)
        except Exception:
            log.debug("Failed to connect splitterMoved signal", exc_info=True)

    # ---- table sync (kept identical) ----

    def _on_splitter_moved(self, _pos: int, _index: int) -> None:
        try:
            ba = self.tree_folder_splitter.saveState()
            self.main_config.gui.splitter_state = bytes(ba.data())
        except Exception:
            log.debug("Failed to save splitter state", exc_info=True)

    def _connect_table_scrollbars(self) -> None:
        sb_a = self.tableViewCurrent.verticalScrollBar()
        sb_b = self.tableViewRenamed.verticalScrollBar()
        self._syncing_scrollbars = False

        def sync(dst, value: int) -> None:
            if self._syncing_scrollbars:
                return
            if dst.value() == value:
                return
            self._syncing_scrollbars = True
            try:
                dst.setValue(value)
            finally:
                self._syncing_scrollbars = False

        sb_a.valueChanged.connect(lambda v: sync(sb_b, v))
        sb_b.valueChanged.connect(lambda v: sync(sb_a, v))

        hsb_a = self.tableViewCurrent.horizontalScrollBar()
        hsb_b = self.tableViewRenamed.horizontalScrollBar()
        hsb_a.valueChanged.connect(lambda v: sync(hsb_b, v))
        hsb_b.valueChanged.connect(lambda v: sync(hsb_a, v))

    def _connect_table_selection(self) -> None:
        sel = QItemSelectionModel(self.rename_model)
        self.tableViewCurrent.setSelectionModel(sel)
        self.tableViewRenamed.setSelectionModel(sel)

        def ensure_visible(current, previous) -> None:
            if current.isValid():
                self.tableViewCurrent.scrollTo(current, QAbstractItemView.ScrollHint.EnsureVisible)
                self.tableViewRenamed.scrollTo(current, QAbstractItemView.ScrollHint.EnsureVisible)

        sel.currentChanged.connect(ensure_visible)

        def on_selection_changed(_selected, _deselected) -> None:
            if bool(self.main_config.app.only_rename_selected_files):
                self._recalculate_preview_for_current_items()

        sel.selectionChanged.connect(on_selection_changed)

    # ---- preview / rename ops ----

    def _reset_preview_to_original(self, items: list[FileInfo]) -> None:
        for fi in items:
            fi.proposed_name = fi.filename
            fi.proposed_suffix = fi.suffix if not fi.is_folder else ""

    def _apply_preview_to_selected_rows(self, items: list[FileInfo], rows: list[int]) -> None:
        if not rows:
            return
        unique_rows = sorted({int(r) for r in rows if int(r) >= 0})
        selected_items: list[FileInfo] = []
        for r in unique_rows:
            if 0 <= r < len(items):
                selected_items.append(items[r])
        if not selected_items:
            return
        from ...services.renamer import preview

        preview(selected_items, self.main_config)

    def _wire_table_sort_persistence(self) -> None:
        self._syncing_sort = False
        hdr_a = self.tableViewCurrent.horizontalHeader()
        hdr_b = self.tableViewRenamed.horizontalHeader()

        def on_sort_changed(section: int, order: Qt.SortOrder) -> None:
            if self._syncing_sort:
                return
            self._syncing_sort = True
            try:
                self.main_config.gui.table_sort_column = int(section)
                self.main_config.gui.table_sort_descending = (order == Qt.SortOrder.DescendingOrder)
                hdr_a.blockSignals(True)
                hdr_b.blockSignals(True)
                try:
                    hdr_a.setSortIndicator(section, order)
                    hdr_b.setSortIndicator(section, order)
                finally:
                    hdr_a.blockSignals(False)
                    hdr_b.blockSignals(False)
                self.tableViewCurrent.sortByColumn(section, order)
                self.tableViewRenamed.sortByColumn(section, order)
            finally:
                self._syncing_sort = False

        hdr_a.sortIndicatorChanged.connect(on_sort_changed)
        hdr_b.sortIndicatorChanged.connect(on_sort_changed)

    def _apply_persisted_table_sorting(self) -> None:
        section = int(self.main_config.gui.table_sort_column)
        order = Qt.SortOrder.DescendingOrder if self.main_config.gui.table_sort_descending else Qt.SortOrder.AscendingOrder
        hdr_a = self.tableViewCurrent.horizontalHeader()
        hdr_b = self.tableViewRenamed.horizontalHeader()
        hdr_a.blockSignals(True)
        hdr_b.blockSignals(True)
        try:
            hdr_a.setSortIndicator(section, order)
            hdr_b.setSortIndicator(section, order)
        finally:
            hdr_a.blockSignals(False)
            hdr_b.blockSignals(False)
        self.tableViewCurrent.sortByColumn(section, order)
        self.tableViewRenamed.sortByColumn(section, order)

    def _wire_signals(self) -> None:
        self.settings_button.clicked.connect(self.open_settings_dialog)
        self.help_button.clicked.connect(self.open_help_dialog)
        self.about_button.clicked.connect(self.open_about_dialog)
        self.back_button.clicked.connect(self.go_back)
        self.forward_button.clicked.connect(self.go_forward)
        self.show_folders_button.toggled.connect(self._on_show_folders_toggled)
        self.show_files_button.toggled.connect(self._on_show_files_toggled)
        self.load_subfolders_button.toggled.connect(self._on_load_subfolders_toggled)
        try:
            self.regex_reference_button.clicked.connect(lambda: toggle_regex_reference_dock(self._regex_reference_dock))
        except Exception:
            log.debug("Failed to connect regex reference button", exc_info=True)
        try:
            self._regex_reference_dock.visibilityChanged.connect(self.regex_reference_button.setChecked)
        except Exception:
            log.debug("Failed to connect regex reference dock visibility", exc_info=True)
        self.parent_dir_button.clicked.connect(self.go_to_parent_folder)
        self.tree_view.clicked.connect(self.on_tree_item_clicked)
        self.tableViewCurrent.doubleClicked.connect(self.on_current_table_double_clicked)
        self.file_mask.textChanged.connect(self._on_file_mask_text_changed)
        self.file_mask.returnPressed.connect(self._reload_for_current_file_mask)
        self.file_mask.historyItemRemoved.connect(self._on_file_mask_history_removed)
        self.current_path.returnPressed.connect(lambda: self.set_active_folder(self.current_path.text()))
        self.current_path.historyItemRemoved.connect(self._on_path_history_removed)
        self._regex_pattern_edit.historyItemRemoved.connect(self._on_regex_pattern_history_removed)
        self._regex_replacement_edit.historyItemRemoved.connect(self._on_regex_replacement_history_removed)
        self.load_folder_contents()

    def _open_non_modal_dialog(
        self,
        *,
        attr_name: str,
        button: QPushButton,
        factory,
    ) -> None:
        """Open/close a non-modal dialog and keep the opener button pressed.

        Behavior:
        - If dialog is open: close it (so rapid repeated presses stay in sync).
        - If dialog is closed: open it.
        - While open: keep `button` checked; uncheck on close.

        `attr_name` stores the dialog instance so it can be re-used.
        """

        # Make the button behave like a toggle tied to dialog visibility.
        try:
            button.setCheckable(True)
        except Exception:
            log.debug("Failed to set button checkable", exc_info=True)

        dlg = getattr(self, attr_name, None)
        if dlg is not None:
            try:
                if dlg.isVisible():
                    # Toggle behavior: pressing again closes the currently-open dialog.
                    try:
                        dlg.close()
                    finally:
                        return
            except Exception:
                log.debug("Failed to check dialog visibility", exc_info=True)

        dlg = factory()
        setattr(self, attr_name, dlg)

        try:
            button.setChecked(True)
        except Exception:
            log.debug("Failed to set button checked", exc_info=True)

        # When the dialog closes, unpress the button and release our reference.
        def _on_finished(_result: int) -> None:
            try:
                button.setChecked(False)
            except Exception:
                log.debug("Failed to uncheck button on dialog close", exc_info=True)
            try:
                if getattr(self, attr_name, None) is dlg:
                    setattr(self, attr_name, None)
            except Exception:
                log.debug("Failed to clear dialog reference on close", exc_info=True)

        try:
            dlg.finished.connect(_on_finished)
        except Exception:
            log.debug("Failed to connect dialog finished signal", exc_info=True)
            # Fallback for non-QDialog-like widgets.
            try:
                dlg.destroyed.connect(lambda *_: _on_finished(0))
            except Exception:
                log.debug("Failed to connect dialog destroyed signal", exc_info=True)

        try:
            dlg.show()
            dlg.raise_()
            dlg.activateWindow()
        except Exception:
            log.debug("Failed to show dialog non-modally", exc_info=True)
            # Last resort: fall back to modal exec.
            try:
                dlg.exec()
            except Exception:
                log.debug("Failed to show dialog modally as fallback", exc_info=True)

    def open_about_dialog(self) -> None:
        self._open_non_modal_dialog(
            attr_name="_about_dialog",
            button=self.about_button,
            factory=lambda: AboutDialog(self),
        )

    def open_help_dialog(self) -> None:
        self._open_non_modal_dialog(
            attr_name="_help_dialog",
            button=self.help_button,
            factory=lambda: HelpDialog(self),
        )

    # NOTE: `open_settings_dialog()` is implemented further below (existing
    # method kept for additional side effects after closing the dialog).

    def _on_rules_changed(self) -> None:
        timer = getattr(self, "_rules_preview_timer", None)
        if timer is None:
            return
        timer.stop()
        timer.start(200)

    def _sync_regex_rows_visibility(self) -> None:
        enabled = bool(getattr(self.main_config.app, "enable_regex_rename_option", False))
        btn = getattr(self, "regex_reference_button", None)
        if btn is not None:
            try:
                btn.setVisible(enabled)
            except Exception:
                log.debug("Failed to set regex reference button visibility", exc_info=True)
        cb = getattr(self, "_regex_name_row", None)
        if cb is not None:
            try:
                cb.setVisible(enabled)
                p = cb.parent()
                if p is not None and isinstance(p, QWidget):
                    p.setVisible(enabled)
            except Exception:
                log.debug("Failed to set regex name row visibility", exc_info=True)
        try:
            self._update_rules_tabs_height()
        except Exception:
            log.debug("Failed to update rules tabs height", exc_info=True)
        if not enabled:
            try:
                if bool(getattr(self.main_config.regex, "is_active_regex_rename", False)):
                    self.main_config.regex.is_active_regex_rename = False
            except Exception:
                log.debug("Failed to deactivate regex rename", exc_info=True)
            try:
                self._regex_reference_dock.hide()
            except Exception:
                log.debug("Failed to hide regex reference dock", exc_info=True)
            try:
                if btn is not None:
                    btn.setChecked(False)
            except Exception:
                log.debug("Failed to uncheck regex reference button", exc_info=True)

    def _recalculate_preview_for_current_items(self) -> None:
        items = self.rename_model.items()
        if not items:
            return
        if bool(self.main_config.app.only_rename_selected_files):
            self._reset_preview_to_original(items)
            self._apply_preview_to_selected_rows(items, self._selected_rows())
        else:
            from ...services.renamer import preview

            preview(items, self.main_config)
        self.rename_model.notify_proposed_changed()
        try:
            hdr = self.tableViewCurrent.horizontalHeader()
            self.rename_model.sort(int(hdr.sortIndicatorSection()), hdr.sortIndicatorOrder())
        except Exception:
            log.debug("Failed to sort table after preview recalculation", exc_info=True)
        self._schedule_resize_proposed_columns_to_contents()
        self._update_rename_plan_info()

    def _update_rename_plan_info(self) -> None:
        items = list(self.rename_model.items())
        suffix_note = ""
        if bool(self.main_config.app.only_rename_selected_files):
            rows = self._selected_rows()
            if not rows:
                items = []
                suffix_note = " (no rows selected)"
            else:
                unique_rows = sorted({int(r) for r in rows})
                selected_items = [self.rename_model.get_item(r) for r in unique_rows]
                items = [fi for fi in selected_items if fi is not None]
        ops = self._rename_ops_from_items(items)
        ops_count = int(len(ops))
        self.rename_plan_info.setText(f"Will rename: {ops_count} item(s){suffix_note}")
        try:
            if ops_count > 0:
                # Reset to default so the palette/style sheet can choose the normal label color.
                # Setting an explicit color here makes it hard to distinguish the "0" state.
                self.rename_plan_info.setStyleSheet("")
            else:
                # When there is nothing to rename, match the look of labels inside a
                # disabled group box (e.g. the Auto number change details). Qt applies
                # the disabled palette group based on *enabled state*, not just a QSS
                # palette() expression, so we toggle the label enabled state.
                self.rename_plan_info.setEnabled(False)
        except Exception:
            log.debug("Failed to style rename plan info label", exc_info=True)

        # Ensure we re-enable the label when needed (kept out of the try so we don't
        # accidentally leave it disabled on exceptions).
        if ops_count > 0:
            self.rename_plan_info.setEnabled(True)

    def _schedule_resize_proposed_columns_to_contents(self) -> None:
        try:
            self._proposed_columns_resize_timer.stop()
            self._proposed_columns_resize_timer.start(50)
        except Exception:
            log.debug("Failed to schedule proposed columns resize", exc_info=True)

    def _resize_proposed_columns_to_contents(self) -> None:
        try:
            self.tableViewRenamed.resizeColumnToContents(RenameTableModel.NEW_NAME_COL)
            self.tableViewRenamed.resizeColumnToContents(RenameTableModel.NEW_EXT_COL)
        except Exception:
            log.debug("Failed to resize proposed columns to contents", exc_info=True)

    def _selected_rows(self) -> list[int]:
        sel_model = self.tableViewCurrent.selectionModel()
        if sel_model is None:
            return []
        return [int(mi.row()) for mi in sel_model.selectedRows()]

    def _rename_ops_from_items(self, items: list[FileInfo]) -> list[RenameOp]:
        ops: list[RenameOp] = []
        for fi in items:
            if fi.is_folder and not bool(self.main_config.app.rename_folders):
                continue
            old_path = Path(fi.path)
            parent = old_path.parent
            if fi.is_folder:
                new_basename = str(fi.proposed_name)
            else:
                if fi.proposed_suffix:
                    new_basename = f"{fi.proposed_name}.{fi.proposed_suffix}"
                else:
                    new_basename = str(fi.proposed_name)
            new_path = parent / new_basename
            if str(old_path) == str(new_path):
                continue
            ops.append(RenameOp(old_path=old_path, new_path=new_path, label=f"Rename {old_path.name}"))
        ops.sort(key=lambda op: str(op.old_path))
        ops.sort(key=lambda op: op.old_path.as_posix().count("/"), reverse=True)
        return ops

    def _validate_rename_ops(self, ops: list[RenameOp]) -> None:
        seen: dict[str, RenameOp] = {}
        for op in ops:
            k = os.path.normcase(str(op.new_path))
            prev = seen.get(k)
            if prev is not None and os.path.normcase(str(prev.old_path)) != k:
                raise RuntimeError(f"Multiple entries would be renamed to: {op.new_path}")
            seen[k] = op
        for op in ops:
            if os.path.normcase(str(op.old_path)) == os.path.normcase(str(op.new_path)):
                continue
            if os.path.lexists(str(op.new_path)):
                raise RuntimeError(f"Destination already exists: {op.new_path}")

    def _on_rename_clicked(self) -> None:
        try:
            if bool(getattr(self.main_config.app, "enable_regex_rename_option", False)) and bool(
                getattr(self.main_config.regex, "is_active_regex_rename", False)
            ):
                import re

                pat = str(getattr(self.main_config.regex, "regex_pattern", "") or "")
                repl = str(getattr(self.main_config.regex, "regex_replacement", "") or "")
                if not pat.strip():
                    raise RuntimeError("Regex pattern is empty.")
                compiled = re.compile(pat)
                compiled.sub(repl, "test")
        except Exception as e:
            log.exception("Regex rename error: %s", e)
            _make_msg_box(
                QMessageBox.Icon.Critical,
                self,
                "Regex rename error",
                "Regex renaming is enabled, but the pattern/replacement is invalid.\n\n" f"{e}\n\n" "No files were renamed.",
            ).exec()
            return

        items = list(self.rename_model.items())
        if bool(self.main_config.app.only_rename_selected_files):
            rows = self._selected_rows()
            if not rows:
                _make_msg_box(QMessageBox.Icon.Information, self, "Rename", "No row is selected.").exec()
                return
            unique_rows = sorted({int(r) for r in rows})
            selected_items = [self.rename_model.get_item(r) for r in unique_rows]
            items = [fi for fi in selected_items if fi is not None]

        ops = self._rename_ops_from_items(items)
        if not ops:
            _make_msg_box(QMessageBox.Icon.Information, self, "Rename", "No changes to apply.").exec()
            return
        if bool(self.main_config.app.ask_confirmation_before_renaming):
            msg = f"Rename {len(ops)} item(s)?"
            box = _make_msg_box(
                QMessageBox.Icon.Question,
                self,
                "Confirm rename",
                msg,
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            )
            if box.exec() != QMessageBox.StandardButton.Yes:
                return
        try:
            self._validate_rename_ops(ops)
        except Exception as e:
            log.exception("Rename validation failed: %s", e)
            _make_msg_box(QMessageBox.Icon.Critical, self, "Rename failed", str(e)).exec()
            return

        log.info("Rename started: %d operation(s) in %s", len(ops), self.current_path.text())
        self._undo_stack.beginMacro("Rename")
        renamed_count = 0
        fully_successful = False
        try:
            for op in ops:
                try:
                    rename_path(op.old_path, op.new_path)
                except Exception as e:
                    log.exception(
                        "Rename failed: %s -> %s after %d successful rename(s)",
                        op.old_path, op.new_path, renamed_count,
                    )
                    _make_msg_box(
                        QMessageBox.Icon.Critical,
                        self,
                        "Rename failed",
                        f"Stopped after {renamed_count} successful rename(s).\n\n{op.old_path} → {op.new_path}\n\nError: {e}",
                    ).exec()
                    break
                renamed_count += 1
                log.debug("Renamed: %s -> %s", op.old_path, op.new_path)
                self._undo_stack.push(RenameCommand(op.old_path, op.new_path, label=op.label))
            fully_successful = (renamed_count == len(ops))
        finally:
            self._undo_stack.endMacro()

        if fully_successful:
            log.info("Rename completed successfully: %d item(s)", renamed_count)
            self._save_regex_history_if_active()
            self._save_text_field_histories_after_rename()
        else:
            log.warning("Rename partially completed: %d of %d item(s)", renamed_count, len(ops))

        if fully_successful and bool(self.main_config.app.uncheck_after_renaming):
            self._uncheck_rules_after_successful_rename()
        self.load_folder_contents()

    def _on_path_history_removed(self, path: str) -> None:
        """Remove a path from the recent-paths history and persist the change."""
        self.main_config.app.remove_recent_path(path)
        self.current_path.set_history(self.main_config.app.recent_paths)

    def _on_file_mask_history_removed(self, mask: str) -> None:
        """Remove a file mask from the recent-masks history and persist the change."""
        self.main_config.app.remove_recent_mask(mask)
        self.file_mask.set_history(self.main_config.app.recent_masks)

    def _on_keep_these_history_removed(self, keep_these: str) -> None:
        """Remove a keep-these value from recent history and persist the change."""
        self.main_config.app.remove_recent_keep_these(keep_these)
        self._refresh_keep_these_histories()

    def _refresh_keep_these_histories(self) -> None:
        """Apply persisted keep-these history to all visible keep-these editors."""
        try:
            for editor in self.findChildren(HistoryLineEdit):
                if editor.placeholderText() == "Alphanumeric characters to keep":
                    editor.set_history(self.main_config.app.recent_keep_these)
        except Exception:
            log.debug("Failed to refresh keep-these histories", exc_info=True)

    def _save_keep_these_history(self, editor: QLineEdit) -> None:
        """Record the current keep-these value into recent history and refresh editors."""
        value = editor.text().strip()
        if not value:
            return
        self.main_config.app.add_recent_keep_these(value)
        self._refresh_keep_these_histories()

    def _on_text_field_history_removed(self, target: TextTransformConfig, history_attr: str, value: str) -> None:
        """Remove *value* from a TextTransformConfig history list and refresh matching editors."""
        remove_method_name = f"remove_{history_attr}"
        remove_fn = getattr(target, remove_method_name, None)
        if remove_fn is not None:
            remove_fn(value)
        else:
            # Fallback: remove directly from the list
            hist = getattr(target, history_attr, [])
            try:
                hist.remove(value)
            except ValueError:
                pass
        self._refresh_text_field_histories(target, history_attr)

    def _save_text_field_history(self, editor: QLineEdit, target: TextTransformConfig, history_attr: str) -> None:
        """Record the current text field value into the corresponding recent-history list."""
        value = editor.text().strip()
        if not value:
            return
        add_method_name = f"add_{history_attr}"
        add_fn = getattr(target, add_method_name, None)
        if add_fn is not None:
            add_fn(value)
        else:
            # Fallback: add directly to the list
            hist = getattr(target, history_attr, [])
            if value in hist:
                hist.remove(value)
            hist.insert(0, value)
        self._refresh_text_field_histories(target, history_attr)

    def _refresh_text_field_histories(self, target: TextTransformConfig, history_attr: str) -> None:
        """Refresh all HistoryLineEdit widgets that share the same target and history_attr."""
        try:
            history = getattr(target, history_attr, [])
            for editor in self.findChildren(HistoryLineEdit):
                # Match editors by checking the stored connection info via the property
                ed_target = getattr(editor, '_history_target', None)
                ed_attr = getattr(editor, '_history_attr', None)
                if ed_target is target and ed_attr == history_attr:
                    editor.set_history(history)
        except Exception:
            log.debug("Failed to refresh text field histories for %s", history_attr, exc_info=True)

    def _save_text_field_histories_after_rename(self) -> None:
        """Record current text field values into history after a successful rename."""
        try:
            for cfg in (self.main_config.name, self.main_config.ext):
                if cfg.is_active_text_to_replace:
                    if cfg.text_to_replace.strip():
                        cfg.add_recent_text_to_replace(cfg.text_to_replace)
                    if cfg.text_for_replacing.strip():
                        cfg.add_recent_text_for_replacing(cfg.text_for_replacing)
                if cfg.is_active_insert_before and cfg.insert_before.strip():
                    cfg.add_recent_insert_before(cfg.insert_before)
                if cfg.is_active_insert_after and cfg.insert_after.strip():
                    cfg.add_recent_insert_after(cfg.insert_after)
                if cfg.is_active_insert_at_position and cfg.insert_what.strip():
                    cfg.add_recent_insert_what(cfg.insert_what)
            # Refresh all history editors
            for cfg in (self.main_config.name, self.main_config.ext):
                for attr in (
                    "recent_text_to_replace",
                    "recent_text_for_replacing",
                    "recent_insert_before",
                    "recent_insert_after",
                    "recent_insert_what",
                ):
                    self._refresh_text_field_histories(cfg, attr)
        except Exception:
            log.debug("Failed to save text field histories after rename", exc_info=True)

    def _on_regex_pattern_history_removed(self, pattern: str) -> None:
        """Remove a regex pattern from the recent-patterns history and persist the change."""
        self.main_config.regex.remove_recent_pattern(pattern)
        self._regex_pattern_edit.set_history(self.main_config.regex.recent_regex_patterns)

    def _on_regex_replacement_history_removed(self, replacement: str) -> None:
        """Remove a regex replacement from the recent-replacements history and persist the change."""
        self.main_config.regex.remove_recent_replacement(replacement)
        self._regex_replacement_edit.set_history(self.main_config.regex.recent_regex_replacements)

    def _save_regex_history_if_active(self) -> None:
        """Record the current regex pattern/replacement in the recent-history lists."""
        try:
            if not (
                bool(getattr(self.main_config.app, "enable_regex_rename_option", False))
                and bool(getattr(self.main_config.regex, "is_active_regex_rename", False))
            ):
                return
            pat = str(getattr(self.main_config.regex, "regex_pattern", "") or "").strip()
            repl = str(getattr(self.main_config.regex, "regex_replacement", "") or "").strip()
            if pat:
                self.main_config.regex.add_recent_pattern(pat)
                self._regex_pattern_edit.set_history(self.main_config.regex.recent_regex_patterns)
            if repl:
                self.main_config.regex.add_recent_replacement(repl)
                self._regex_replacement_edit.set_history(self.main_config.regex.recent_regex_replacements)
        except Exception:
            log.debug("Failed to save regex history", exc_info=True)

    def _uncheck_rules_after_successful_rename(self) -> None:
        for cb in list(self._uncheck_after_rename_checkboxes):
            try:
                if cb is not None and cb.isChecked():
                    cb.setChecked(False)
            except Exception:
                log.debug("Failed to uncheck rule checkbox after rename", exc_info=True)

    def _on_undo_clicked(self) -> None:
        if not self._undo_stack.canUndo():
            return
        try:
            self._undo_stack.undo()
            log.info("Undo completed successfully")
        except Exception as e:
            log.exception("Undo failed: %s", e)
            _make_msg_box(QMessageBox.Icon.Critical, self, "Undo failed", str(e)).exec()
        finally:
            self.load_folder_contents()

    # ---- settings/about ----

    def open_settings_dialog(self) -> None:
        # Non-modal settings: keep Settings button pressed while the dialog is open,
        # and perform the same post-close refresh work when it is closed.

        def _after_closed() -> None:
            if self.main_config.app.remember_last_path:
                new_path = self.main_config.app.current_path
                if new_path and new_path != self.current_path.text():
                    self.set_active_folder(new_path)
                    return
            self.load_folder_contents()
            self._sync_regex_rows_visibility()

        # Avoid accumulating multiple `.finished.connect(...)` handlers if the
        # Settings button is pressed repeatedly.
        try:
            if hasattr(self, "_settings_after_close_connected") and bool(getattr(self, "_settings_after_close_connected")):
                pass
        except Exception:
            log.debug("Failed to check settings dialog connection state", exc_info=True)

        self._open_non_modal_dialog(
            attr_name="_settings_dialog",
            button=self.settings_button,
            factory=lambda: SettingsDialog(self, self.main_config),
        )

        # Attach after-close behavior to the active dialog (if it exists).
        dlg = getattr(self, "_settings_dialog", None)
        if dlg is not None:
            try:
                # Ensure we only connect once per dialog instance.
                already = bool(getattr(dlg, "_after_close_hook_installed", False))
                if not already:
                    dlg._after_close_hook_installed = True  # type: ignore[attr-defined]
                    dlg.finished.connect(lambda _r: _after_closed())
            except Exception:
                log.debug("Failed to connect settings dialog after-close hook", exc_info=True)

    # ---- mask reload + toggles ----

    def _on_file_mask_text_changed(self, _text: str) -> None:
        self._file_mask_reload_timer.stop()
        self._file_mask_reload_timer.start(300)

    def _reload_for_current_file_mask(self) -> None:
        mask = self.file_mask.text()
        self.main_config.app.file_mask = mask
        if mask.strip():
            self.main_config.app.add_recent_mask(mask.strip())
            self.file_mask.set_history(self.main_config.app.recent_masks)
        self.load_folder_contents()

    def _on_show_folders_toggled(self, v: bool) -> None:
        self.main_config.app.show_folders = bool(v)
        self.load_folder_contents()

    def _on_show_files_toggled(self, v: bool) -> None:
        self.main_config.app.show_files = bool(v)
        self.load_folder_contents()

    def _on_load_subfolders_toggled(self, v: bool) -> None:
        self.main_config.app.load_subfolders = bool(v)
        self.load_folder_contents()

    # ---- navigation ----

    def on_current_table_double_clicked(self, index: QtCore.QModelIndex) -> None:
        if not index.isValid():
            return
        item = self.rename_model.get_item(index.row())
        if item is None or not item.is_folder:
            return
        self.set_active_folder(item.path)

    def set_active_folder(self, folder_path: str) -> None:
        self._set_active_folder(folder_path, record_history=True)

    def _set_active_folder(self, folder_path: str, *, record_history: bool) -> None:
        if not folder_path:
            return
        folder_path = os.path.abspath(str(folder_path))
        if not os.path.isdir(folder_path):
            return
        log.info("Folder changed: %s", folder_path)
        prev_path = (self.main_config.app.current_path or "").strip()
        prev_path = os.path.abspath(prev_path) if prev_path else ""
        if record_history and prev_path and prev_path != folder_path and os.path.isdir(prev_path):
            self._nav_back_stack.append(prev_path)
            self._nav_forward_stack.clear()
        self.current_path.setText(folder_path)
        self.current_q_dir = QDir(folder_path)
        self.main_config.app.current_path = folder_path
        self.main_config.app.add_recent_path(folder_path)
        self.current_path.set_history(self.main_config.app.recent_paths)
        tree_index_src = self.filesystem_model.index(folder_path)
        tree_index = self.filesystem_sort_proxy.mapFromSource(tree_index_src)
        if tree_index.isValid():
            self.tree_view.setExpanded(tree_index, True)
            self.tree_view.setCurrentIndex(tree_index)
            self.tree_view.scrollTo(tree_index, QAbstractItemView.ScrollHint.PositionAtCenter)
        self.load_folder_contents()
        self._update_navigation_buttons()

    def _update_navigation_buttons(self) -> None:
        try:
            self.back_button.setEnabled(bool(self._nav_back_stack))
            self.forward_button.setEnabled(bool(self._nav_forward_stack))
        except Exception:
            log.debug("Failed to update navigation buttons", exc_info=True)

    def go_back(self) -> None:
        if not self._nav_back_stack:
            self._update_navigation_buttons()
            return
        current = self.current_path.text().strip()
        current = os.path.abspath(current) if current else ""
        target = self._nav_back_stack.pop()
        if current and os.path.isdir(current):
            self._nav_forward_stack.append(current)
        self._set_active_folder(target, record_history=False)
        self._update_navigation_buttons()

    def go_forward(self) -> None:
        if not self._nav_forward_stack:
            self._update_navigation_buttons()
            return
        current = self.current_path.text().strip()
        current = os.path.abspath(current) if current else ""
        target = self._nav_forward_stack.pop()
        if current and os.path.isdir(current):
            self._nav_back_stack.append(current)
        self._set_active_folder(target, record_history=False)
        self._update_navigation_buttons()

    def go_to_parent_folder(self) -> None:
        current_path = self.current_path.text()
        parent_folder = os.path.dirname(current_path)
        self.set_active_folder(parent_folder)

    def on_tree_item_clicked(self, index) -> None:
        self.tree_view.resizeColumnToContents(0)
        # Tree view now uses a sort proxy model; map indices back to the source model.
        index_src = self.filesystem_sort_proxy.mapToSource(index)
        file_info = self.filesystem_model.fileInfo(index_src)
        if file_info.isDir():
            folder_path = file_info.absoluteFilePath()
            self.set_active_folder(folder_path)

    # ---- loader ----

    def _wire_loader_signals(self) -> None:
        self._loader.load_started.connect(self._on_load_started)
        self._loader.load_progress.connect(self._on_load_progress)
        self._loader.load_finished.connect(self._on_load_finished)
        self._loader.load_failed.connect(self._on_load_failed)
        self._loader.load_canceled.connect(self._on_load_canceled)

    # centers the view in the folder tree to current working folder
    def scroll_to_last_path(self) -> None:
        # this must be run after some time from the start, otherwise it does not work
        QtCore.QCoreApplication.processEvents()
        current_index_model = self.filesystem_model.index(self.main_config.app.current_path)
        current_index_proxy = self.filesystem_sort_proxy.mapFromSource(current_index_model)
        self.tree_view.scrollTo(current_index_proxy, QAbstractItemView.ScrollHint.PositionAtCenter)

    def load_folder_contents(self) -> None:
        base_path = self.current_path.text()
        opts = ScanOptions(
            mask=self.file_mask.text() or self.main_config.app.file_mask,
            include_folders=self.main_config.app.show_folders,
            include_files=self.main_config.app.show_files,
            recursive=self.main_config.app.load_subfolders,
            case_sensitive=self.main_config.app.file_mask_case_sensitive,
        )
        request = LoadRequest(Path(base_path), opts, self.main_config)
        self._active_job_id = self._loader.load(request)

    def _on_load_started(self, job_id: int) -> None:
        self._active_job_id = job_id
        if not self._busy_cursor_active:
            QtWidgets.QApplication.setOverrideCursor(Qt.CursorShape.WaitCursor)
            self._busy_cursor_active = True
        self._progress_timer.stop()
        self._progress_timer.timeout.connect(lambda: self._maybe_show_progress_dialog(job_id))
        self._progress_timer.start(1000)

    def _on_load_progress(self, job_id: int, scanned_count: int) -> None:
        if self._active_job_id != job_id:
            return

    def _on_load_finished(self, job_id: int, files: list) -> None:
        if self._active_job_id != job_id:
            return
        self._teardown_progress_ui()
        try:
            if bool(self.main_config.app.only_rename_selected_files):
                self._reset_preview_to_original(files)
            else:
                from ...services.renamer import preview

                preview(files, self.main_config)
        except Exception:
            log.debug("Failed to compute rename preview", exc_info=True)
        self.rename_model.set_items(files)
        self._apply_persisted_table_sorting()
        self.tableViewCurrent.resizeColumnsToContents()
        self.tableViewRenamed.resizeColumnsToContents()
        if bool(self.main_config.app.only_rename_selected_files):
            self._recalculate_preview_for_current_items()
        self._update_rename_plan_info()

    def _on_load_failed(self, job_id: int, error_text: str) -> None:
        if self._active_job_id != job_id:
            return
        self._teardown_progress_ui()
        _make_msg_box(QMessageBox.Icon.Critical, self, "Load failed", error_text).exec()

    def _on_load_canceled(self, job_id: int) -> None:
        if self._active_job_id != job_id:
            return
        self._teardown_progress_ui()

    def _teardown_progress_ui(self) -> None:
        self._progress_timer.stop()
        try:
            self._progress_timer.timeout.disconnect()
        except TypeError:
            log.debug("Failed to disconnect progress timer", exc_info=True)
        if self._progress_dialog is not None:
            self._progress_dialog.close()
            self._progress_dialog.deleteLater()
            self._progress_dialog = None
        if self._busy_cursor_active:
            QtWidgets.QApplication.restoreOverrideCursor()
            self._busy_cursor_active = False

    def _maybe_show_progress_dialog(self, job_id: int) -> None:
        if self._active_job_id != job_id:
            return
        if self._progress_dialog is not None:
            return
        dlg = QDialog(self)
        dlg.setWindowTitle("Loading folder…")
        dlg.setModal(True)
        dlg.setWindowFlags(Qt.WindowType.Dialog | Qt.WindowType.FramelessWindowHint)
        layout = QtWidgets.QVBoxLayout(dlg)  # type: ignore
        layout.setContentsMargins(12, 12, 12, 12)
        title = QtWidgets.QLabel("Loading folder…", dlg)
        title.setAlignment(Qt.AlignmentFlag.AlignLeft)
        layout.addWidget(title)
        bar = QProgressBar(dlg)
        bar.setRange(0, 0)
        layout.addWidget(bar)
        btn_row = QHBoxLayout()
        cancel_btn = QPushButton("&Cancel", dlg)
        fix_mnemonic_for_mac(cancel_btn)
        cancel_btn.clicked.connect(lambda: self._loader.cancel(job_id))
        btn_row.addStretch(1)
        btn_row.addWidget(cancel_btn)
        layout.addLayout(btn_row)  # type: ignore
        self._progress_dialog = dlg
        dlg.show()

    def _sync_right_groupbox_widths(self) -> None:
        if not hasattr(self, "numbering_group") or not hasattr(self, "rename_undo_group"):
            return
        num = getattr(self, "numbering_group", None)
        ren = getattr(self, "rename_undo_group", None)
        if num is None or ren is None:
            return
        width = max(int(num.sizeHint().width()), int(ren.sizeHint().width()))
        if width <= 0:
            return
        for gb in (num, ren):
            gb.setMinimumWidth(width)
            gb.setMaximumWidth(width)

    def _update_rules_tabs_height(self) -> None:
        if not hasattr(self, "rules_tabs") or self.rules_tabs is None:
            return
        hint_tabs = int(self.rules_tabs.sizeHint().height())
        hint_num = int(self.numbering_group.sizeHint().height()) if hasattr(self, "numbering_group") else 0
        hint_right = int(self.right_side_groupboxes.sizeHint().height()) if hasattr(self, "right_side_groupboxes") else hint_num
        target_tabs_h = max(hint_tabs, hint_right)
        if target_tabs_h > 0:
            self.rules_tabs.setMinimumHeight(target_tabs_h)
            self.rules_tabs.setMaximumHeight(target_tabs_h)
        if hasattr(self, "numbering_group") and hint_num > 0:
            self.numbering_group.setMaximumHeight(hint_num)
        if hasattr(self, "rules_section"):
            hint = max(target_tabs_h, hint_right)
            if hint > 0:
                self.rules_section.setMaximumHeight(hint)
        self._sync_right_groupbox_widths()

    # ---- close persistence ----

    def closeEvent(self, event) -> None:  # type: ignore[override]
        log.info("Application shutting down")
        if self.main_config.gui.remember_position_and_size:
            if self.isMaximized():
                self.main_config.gui.start_maximized = True
            else:
                self.main_config.gui.start_maximized = False
                g = self.geometry()
                self.main_config.gui.window_position_x = g.x()
                self.main_config.gui.window_position_y = g.y()
                self.main_config.gui.window_size_width = g.width()
                self.main_config.gui.window_size_height = g.height()

        self.main_config.app.current_path = self.current_path.text()
        self.main_config.app.file_mask = self.file_mask.text()
        hdr = self.tableViewCurrent.horizontalHeader()
        self.main_config.gui.table_sort_column = int(hdr.sortIndicatorSection())
        self.main_config.gui.table_sort_descending = (hdr.sortIndicatorOrder() == Qt.SortOrder.DescendingOrder)
        try:
            mw_state = self.saveState()
            try:
                self.main_config.gui.main_window_state = mw_state.data()  # type: ignore[assignment]
            except Exception:
                log.debug("Failed to get window state via .data(), falling back to bytes()", exc_info=True)
                self.main_config.gui.main_window_state = bytes(mw_state)  # type: ignore[arg-type]
        except Exception:
            log.debug("Failed to save main window state on close", exc_info=True)
        self.main_config.save_settings()
        super().closeEvent(event)
