from dataclasses import dataclass
import logging
import os
import sys
from enum import Enum
from typing import Any, Type, TypeVar
from PySide6.QtCore import QSettings, QObject

log = logging.getLogger("filename_ninja.config")

# Helpers for robust typed reads from QSettings
def _get_str(settings: QSettings, key: str, default: str) -> str:
    val = settings.value(key, default)
    if isinstance(val, str):
        return val
    if val is None:
        return default
    return str(val)

def _get_int(settings: QSettings, key: str, default: int) -> int:
    val = settings.value(key, default)
    if isinstance(val, int):
        return val
    if isinstance(val, float):
        return int(val)
    if isinstance(val, str):
        try:
            return int(val)
        except ValueError:
            try:
                return int(float(val))
            except Exception:
                log.debug("Failed to convert setting %r value %r to int", key, val, exc_info=True)
                return default
    try:
        return int(str(val))
    except Exception:
        log.debug("Failed to convert setting %r value %r to int", key, val, exc_info=True)
        return default

def _get_bool(settings: QSettings, key: str, default: bool) -> bool:
    val = settings.value(key, default)
    if isinstance(val, bool):
        return val
    if isinstance(val, int):
        return bool(val)
    if isinstance(val, str):
        s = val.strip().lower()
        if s in ("1", "true", "yes", "on"):
            return True
        if s in ("0", "false", "no", "off"):
            return False
        return default
    return default


TEnum = TypeVar("TEnum", bound=Enum)

def _get_enum(settings: QSettings, key: str, enum_cls: Type[TEnum], default: TEnum) -> TEnum:
    """Read an Enum from QSettings. Accepts stored int value or enum name string."""
    raw: Any = settings.value(key, default.value if isinstance(default, Enum) else default)
    # Try integer conversion first
    try:
        ival = int(raw)
        return enum_cls(ival)  # type: ignore[arg-type]
    except Exception:
        log.debug("Failed to convert setting %r value %r to enum %s by int", key, raw, enum_cls.__name__, exc_info=True)
    # Try by name (string)
    if isinstance(raw, str):
        name = raw.strip()
        try:
            return enum_cls[name]  # type: ignore[index]
        except Exception:
            log.debug("Failed to convert setting %r value %r to enum %s by name", key, raw, enum_cls.__name__, exc_info=True)
    return default


MAX_RECENT_PATHS = 20
MAX_RECENT_MASKS = 20
MAX_RECENT_KEEP_THESE = 20


def _get_str_list(settings: QSettings, key: str, default: list[str] | None = None) -> list[str]:
    """Read a list of strings from QSettings (stored via beginWriteArray)."""
    result: list[str] = []
    size = settings.beginReadArray(key)
    for i in range(size):
        settings.setArrayIndex(i)
        val = settings.value("value", "")
        if isinstance(val, str) and val:
            result.append(val)
    settings.endArray()
    return result if result else (default or [])


def _set_str_list(settings: QSettings, key: str, values: list[str]) -> None:
    """Write a list of strings to QSettings (stored via beginWriteArray)."""
    settings.beginWriteArray(key, len(values))
    for i, v in enumerate(values):
        settings.setArrayIndex(i)
        settings.setValue("value", v)
    settings.endArray()


class CaseChange(Enum):
    LOWERCASE = 0
    UPPERCASE = 1
    EVERY_FIRST_LETTER_UP = 2
    ONLY_FIRST_LETTER_UP = 3


class AutoNumberChange(Enum):
    BEFORE_FILENAME = 0
    AFTER_FILENAME = 1
    BEFORE_EXTENSION = 2
    AFTER_EXTENSION = 3


class ThemeMode(Enum):
    SYSTEM = 0
    LIGHT = 1
    DARK = 2


class LogLevel(Enum):
    ERROR = 40
    WARNING = 30
    INFO = 20
    DEBUG = 10


class LoggingConfig(QObject):
    def __init__(
        self,
        enabled: bool = False,
        level: LogLevel = LogLevel.WARNING,
        log_dir: str = "",
    ) -> None:
        super().__init__()
        self.enabled = enabled
        self.level = level
        # Empty string means "use default log folder".
        self.log_dir = log_dir

    def load_settings(self, settings: QSettings) -> None:
        self.enabled = _get_bool(settings, "logging/enabled", self.enabled)
        self.level = _get_enum(settings, "logging/level", LogLevel, self.level)
        self.log_dir = _get_str(settings, "logging/log_dir", self.log_dir)

    def save_settings(self, settings: QSettings) -> None:
        settings.setValue("logging/enabled", bool(self.enabled))
        settings.setValue("logging/level", int(self.level.value))
        settings.setValue("logging/log_dir", self.log_dir)


class AppConfig(QObject):

    def __init__(self,
                 remember_last_path: bool = True,
                 current_path: str = os.path.expanduser("~"),
                 file_mask: str = "*.*",
                 file_mask_case_sensitive: bool = False,
                 uncheck_after_renaming: bool = False,
                 ask_confirmation_before_renaming: bool = True,
                 show_splash_screen: bool = True,
                 show_folders: bool = True,
                 show_files: bool = True,
                 rename_folders: bool = False,
                 load_subfolders: bool = False,
                 only_rename_selected_files: bool = False,
                 enable_regex_rename_option: bool = False,
                 theme_mode: ThemeMode = ThemeMode.SYSTEM,
                 recent_paths: list[str] | None = None,
                 recent_masks: list[str] | None = None,
                 recent_keep_these: list[str] | None = None):
        
        super().__init__()
            
        self.remember_last_path = remember_last_path
        self.current_path = current_path
        self.file_mask = file_mask
        self.file_mask_case_sensitive = file_mask_case_sensitive
        self.uncheck_after_renaming = uncheck_after_renaming
        self.ask_confirmation_before_renaming = ask_confirmation_before_renaming
        self.show_splash_screen = show_splash_screen
        self.show_folders = show_folders
        self.show_files = show_files
        self.rename_folders = rename_folders
        self.load_subfolders = load_subfolders
        self.only_rename_selected_files = only_rename_selected_files
        # Hidden-by-default advanced option. When False, regex UI controls are not shown.
        self.enable_regex_rename_option = enable_regex_rename_option
        self.theme_mode = theme_mode
        self.recent_paths: list[str] = recent_paths if recent_paths is not None else []
        self.recent_masks: list[str] = recent_masks if recent_masks is not None else []
        self.recent_keep_these: list[str] = recent_keep_these if recent_keep_these is not None else []

    @staticmethod
    def _application_folder() -> str:
        """Return the folder from which the application is running.

        For PyInstaller-frozen executables this is the folder containing the
        executable.  For regular Python invocations it is the current working
        folder.
        """
        if getattr(sys, "frozen", False):
            return os.path.dirname(sys.executable)
        return os.getcwd()

    def load_settings(self, settings: QSettings) -> None:

        self.remember_last_path = _get_bool(settings, "app/remember_last_path", self.remember_last_path)
        if self.remember_last_path:
            self.current_path = _get_str(settings, "app/current_path", self.current_path)

        # Fall back to the application's running folder when the stored path
        # does not point to a valid folder (e.g. it was deleted or the app
        # was moved to a different machine).
        if not os.path.isdir(self.current_path):
            self.current_path = self._application_folder()
            log.info("Stored path is not valid; falling back to application folder: %s", self.current_path)

        self.file_mask = _get_str(settings, "app/file_mask", self.file_mask)
        self.file_mask_case_sensitive = _get_bool(settings, "app/file_mask_case_sensitive", self.file_mask_case_sensitive)
        self.uncheck_after_renaming = _get_bool(settings, "app/uncheck_after_renaming", self.uncheck_after_renaming)
        self.ask_confirmation_before_renaming = _get_bool(settings, "app/ask_confirmation_before_renaming", self.ask_confirmation_before_renaming)
        self.show_splash_screen = _get_bool(settings, "app/show_splash_screen", self.show_splash_screen)
        self.show_folders = _get_bool(settings, "app/show_folders", self.show_folders)
        self.show_files = _get_bool(settings, "app/show_files", self.show_files)
        self.load_subfolders = _get_bool(settings, "app/load_subfolders", self.load_subfolders)
        self.rename_folders = _get_bool(settings, "app/rename_folders", self.rename_folders)
        self.only_rename_selected_files = _get_bool(settings, "app/only_rename_selected_files", self.only_rename_selected_files)
        self.enable_regex_rename_option = _get_bool(settings, "app/enable_regex_rename_option", self.enable_regex_rename_option,
        )

        self.theme_mode = _get_enum(settings, "app/theme_mode", ThemeMode, self.theme_mode)

        self.recent_paths = _get_str_list(settings, "app/recent_paths", self.recent_paths)
        self.recent_masks = _get_str_list(settings, "app/recent_masks", self.recent_masks)
        self.recent_keep_these = _get_str_list(settings, "app/recent_keep_these", self.recent_keep_these)

    def add_recent_path(self, path: str) -> None:
        """Add *path* to the recent-paths list (most-recent first, no duplicates)."""
        path = path.strip()
        if not path:
            return
        if path in self.recent_paths:
            self.recent_paths.remove(path)
        self.recent_paths.insert(0, path)
        self.recent_paths = self.recent_paths[:MAX_RECENT_PATHS]

    def add_recent_mask(self, mask: str) -> None:
        """Add *mask* to the recent-masks list (most-recent first, no duplicates)."""
        mask = mask.strip()
        if not mask:
            return
        if mask in self.recent_masks:
            self.recent_masks.remove(mask)
        self.recent_masks.insert(0, mask)
        self.recent_masks = self.recent_masks[:MAX_RECENT_MASKS]

    def remove_recent_path(self, path: str) -> None:
        """Remove *path* from the recent-paths list."""
        try:
            self.recent_paths.remove(path)
        except ValueError:
            pass

    def remove_recent_mask(self, mask: str) -> None:
        """Remove *mask* from the recent-masks list."""
        try:
            self.recent_masks.remove(mask)
        except ValueError:
            pass

    def add_recent_keep_these(self, keep_these: str) -> None:
        """Add *keep_these* to the recent keep-these list (most-recent first, no duplicates)."""
        keep_these = keep_these.strip()
        if not keep_these:
            return
        if keep_these in self.recent_keep_these:
            self.recent_keep_these.remove(keep_these)
        self.recent_keep_these.insert(0, keep_these)
        self.recent_keep_these = self.recent_keep_these[:MAX_RECENT_KEEP_THESE]

    def remove_recent_keep_these(self, keep_these: str) -> None:
        """Remove *keep_these* from the recent keep-these list."""
        try:
            self.recent_keep_these.remove(keep_these)
        except ValueError:
            pass

    def save_settings(self, settings: QSettings) -> None:
                
        settings.setValue("app/remember_last_path", self.remember_last_path)
        if self.remember_last_path:
            settings.setValue("app/current_path", self.current_path)

        settings.setValue("app/file_mask", self.file_mask)
        settings.setValue("app/file_mask_case_sensitive", self.file_mask_case_sensitive)
        settings.setValue("app/uncheck_after_renaming", self.uncheck_after_renaming)
        settings.setValue("app/ask_confirmation_before_renaming", self.ask_confirmation_before_renaming)
        settings.setValue("app/show_splash_screen", self.show_splash_screen)
        settings.setValue("app/show_folders", self.show_folders)
        settings.setValue("app/show_files", self.show_files)
        settings.setValue("app/load_subfolders", self.load_subfolders)
        settings.setValue("app/rename_folders", self.rename_folders)
        settings.setValue("app/only_rename_selected_files", self.only_rename_selected_files)    
        settings.setValue("app/enable_regex_rename_option", self.enable_regex_rename_option)

        settings.setValue("app/theme_mode", int(self.theme_mode.value))

        _set_str_list(settings, "app/recent_paths", self.recent_paths)
        _set_str_list(settings, "app/recent_masks", self.recent_masks)
        _set_str_list(settings, "app/recent_keep_these", self.recent_keep_these)
        

MAX_RECENT_TEXT_TO_REPLACE = 20
MAX_RECENT_TEXT_FOR_REPLACING = 20
MAX_RECENT_INSERT_BEFORE = 20
MAX_RECENT_INSERT_AFTER = 20
MAX_RECENT_INSERT_WHAT = 20

MAX_RECENT_REGEX_PATTERNS = 20
MAX_RECENT_REGEX_REPLACEMENTS = 20


class RegexRenameConfig(QObject):
    """Configuration for optional regex-based renaming.

    This is intentionally hidden behind an AppConfig option. When enabled in settings,
    the Name/Extension tabs will show these controls.
    """

    def __init__(
        self,
        is_active_regex_rename: bool = False,
        regex_pattern: str = r"^(?P<year>\d{4})_(?P<book>[^-]+)-(?P<author>.+)$",
        regex_replacement: str = r"\g<author> - \g<book> [\g<year>]",
        recent_regex_patterns: list[str] | None = None,
        recent_regex_replacements: list[str] | None = None,
    ) -> None:
        super().__init__()
        self.is_active_regex_rename = is_active_regex_rename
        self.regex_pattern = regex_pattern
        self.regex_replacement = regex_replacement
        self.recent_regex_patterns: list[str] = recent_regex_patterns if recent_regex_patterns is not None else []
        self.recent_regex_replacements: list[str] = recent_regex_replacements if recent_regex_replacements is not None else []

    def load_settings(self, settings: QSettings) -> None:
        self.is_active_regex_rename = _get_bool(
            settings,
            "regex/is_active_regex_rename",
            self.is_active_regex_rename,
        )
        self.regex_pattern = _get_str(settings, "regex/regex_pattern", self.regex_pattern)
        self.regex_replacement = _get_str(settings, "regex/regex_replacement", self.regex_replacement)
        self.recent_regex_patterns = _get_str_list(settings, "regex/recent_patterns", self.recent_regex_patterns)
        self.recent_regex_replacements = _get_str_list(settings, "regex/recent_replacements", self.recent_regex_replacements)

    def add_recent_pattern(self, pattern: str) -> None:
        """Add *pattern* to the recent-patterns list (most-recent first, no duplicates)."""
        pattern = pattern.strip()
        if not pattern:
            return
        if pattern in self.recent_regex_patterns:
            self.recent_regex_patterns.remove(pattern)
        self.recent_regex_patterns.insert(0, pattern)
        self.recent_regex_patterns = self.recent_regex_patterns[:MAX_RECENT_REGEX_PATTERNS]

    def add_recent_replacement(self, replacement: str) -> None:
        """Add *replacement* to the recent-replacements list (most-recent first, no duplicates)."""
        replacement = replacement.strip()
        if not replacement:
            return
        if replacement in self.recent_regex_replacements:
            self.recent_regex_replacements.remove(replacement)
        self.recent_regex_replacements.insert(0, replacement)
        self.recent_regex_replacements = self.recent_regex_replacements[:MAX_RECENT_REGEX_REPLACEMENTS]

    def remove_recent_pattern(self, pattern: str) -> None:
        """Remove *pattern* from the recent-patterns list."""
        try:
            self.recent_regex_patterns.remove(pattern)
        except ValueError:
            pass

    def remove_recent_replacement(self, replacement: str) -> None:
        """Remove *replacement* from the recent-replacements list."""
        try:
            self.recent_regex_replacements.remove(replacement)
        except ValueError:
            pass

    def save_settings(self, settings: QSettings) -> None:
        settings.setValue("regex/is_active_regex_rename", self.is_active_regex_rename)
        settings.setValue("regex/regex_pattern", self.regex_pattern)
        settings.setValue("regex/regex_replacement", self.regex_replacement)
        _set_str_list(settings, "regex/recent_patterns", self.recent_regex_patterns)
        _set_str_list(settings, "regex/recent_replacements", self.recent_regex_replacements)

class GUIConfig(QObject):

    def __init__(self,
                 start_maximized: bool = False,
                 remember_position_and_size: bool = True,
                 window_position_x: int = 100,
                 window_position_y: int = 100,
                 window_size_width: int = 1200,
                 window_size_height : int = 800,
                 table_sort_column: int = 0,
                 table_sort_descending: bool = False,
                 splitter_state: bytes | None = None,
                 main_window_state: bytes | None = None):
        
        super().__init__()    

        self.start_maximized = start_maximized
        self.remember_position_and_size = remember_position_and_size
        self.window_position_x = window_position_x
        self.window_position_y = window_position_y
        self.window_size_width = window_size_width
        self.window_size_height = window_size_height

        # Table sorting (RenameTableModel has 4 columns: 0..3)
        self.table_sort_column = table_sort_column
        self.table_sort_descending = table_sort_descending

        # QSplitter state (stored as hex string in QSettings).
        self.splitter_state: bytes | None = splitter_state

        # QMainWindow state (dock widgets, toolbars). Stored as hex string.
        self.main_window_state: bytes | None = main_window_state

    def load_settings(self, settings: QSettings) -> None:

        self.start_maximized = _get_bool(settings, "gui/start_maximized", self.start_maximized)
        self.remember_position_and_size = _get_bool(settings, "gui/remember_position_and_size", self.remember_position_and_size)
        if self.remember_position_and_size:
            self.window_position_x = _get_int(settings, "gui/position_x", self.window_position_x)
            self.window_position_y = _get_int(settings, "gui/position_y", self.window_position_y)
            self.window_size_width = _get_int(settings, "gui/width", self.window_size_width)
            self.window_size_height = _get_int(settings, "gui/height", self.window_size_height)

        self.table_sort_column = _get_int(settings, "gui/table_sort_column", self.table_sort_column)
        if self.table_sort_column < 0 or self.table_sort_column > 3:
            self.table_sort_column = 0
        self.table_sort_descending = _get_bool(settings, "gui/table_sort_descending", self.table_sort_descending)

        # Splitter state is stored as a hex string for portability.
        raw_state = settings.value("gui/splitter_state", None)
        if isinstance(raw_state, (bytes, bytearray)):
            self.splitter_state = bytes(raw_state)
        elif isinstance(raw_state, str) and raw_state.strip():
            try:
                self.splitter_state = bytes.fromhex(raw_state.strip())
            except Exception:
                log.debug("Failed to decode splitter_state hex string", exc_info=True)
                self.splitter_state = None
        else:
            self.splitter_state = None

        # Main window state (dock layout) stored as hex string for portability.
        raw_mw = settings.value("gui/main_window_state", None)
        if isinstance(raw_mw, (bytes, bytearray)):
            self.main_window_state = bytes(raw_mw)
        elif isinstance(raw_mw, str) and raw_mw.strip():
            try:
                self.main_window_state = bytes.fromhex(raw_mw.strip())
            except Exception:
                log.debug("Failed to decode main_window_state hex string", exc_info=True)
                self.main_window_state = None
        else:
            self.main_window_state = None

    def save_settings(self, settings: QSettings) -> None:

        settings.setValue("gui/start_maximized", self.start_maximized)
        settings.setValue("gui/remember_position_and_size", self.remember_position_and_size)
        if self.remember_position_and_size:
            settings.setValue("gui/position_x", self.window_position_x)
            settings.setValue("gui/position_y", self.window_position_y)
            settings.setValue("gui/width", self.window_size_width)
            settings.setValue("gui/height", self.window_size_height)

        settings.setValue("gui/table_sort_column", int(self.table_sort_column))
        settings.setValue("gui/table_sort_descending", bool(self.table_sort_descending))

        if self.splitter_state:
            try:
                settings.setValue("gui/splitter_state", self.splitter_state.hex())
            except Exception:
                log.warning("Failed to save splitter_state", exc_info=True)

        if self.main_window_state:
            try:
                settings.setValue("gui/main_window_state", self.main_window_state.hex())
            except Exception:
                log.warning("Failed to save main_window_state", exc_info=True)

class TextTransformConfig(QObject):
    """Shared base for name/extension text-transform configuration.

    Subclasses only need to set ``_prefix`` to the QSettings key prefix
    (e.g. ``"name/"`` or ``"ext/"``).
    """

    _prefix: str = ""  # overridden by subclasses

    def __init__(self,
                 is_active_text_to_replace: bool = False,
                 text_to_replace: str = "",
                 text_for_replacing: str = "",
                 text_to_replace_case_sensitive: bool = True,
                 is_active_left_crop_n_characters: bool = False,
                 left_crop_n_characters: int = 0,
                 is_active_right_crop_n_characters: bool = False,
                 right_crop_n_characters: int = 0,
                 is_active_crop_at_position: bool = False,
                 crop_at_position: int = 0,
                 crop_how_many: int = 0,
                 is_active_insert_before: bool = False,
                 insert_before: str = "",
                 is_active_insert_after: bool = False,
                 insert_after: str = "",
                 is_active_insert_at_position: bool = False,
                 insert_at_position: int = 0,
                 insert_what: str = "",
                 is_active_case_change: bool = False,
                 case_change: CaseChange = CaseChange.LOWERCASE,
                 remove_accents: bool = False,
                 remove_non_alphanumeric: bool = False,
                 keep_these_non_alphanumeric: str = "-_ ()[]",
                 recent_text_to_replace: list[str] | None = None,
                 recent_text_for_replacing: list[str] | None = None,
                 recent_insert_before: list[str] | None = None,
                 recent_insert_after: list[str] | None = None,
                 recent_insert_what: list[str] | None = None,):
        
        super().__init__()    

        self.is_active_text_to_replace = is_active_text_to_replace
        self.text_to_replace = text_to_replace
        self.text_for_replacing = text_for_replacing
        self.text_to_replace_case_sensitive = text_to_replace_case_sensitive
        self.is_active_left_crop_n_characters = is_active_left_crop_n_characters
        self.left_crop_n_characters = left_crop_n_characters
        self.is_active_right_crop_n_characters = is_active_right_crop_n_characters
        self.right_crop_n_characters = right_crop_n_characters
        self.is_active_crop_at_position = is_active_crop_at_position
        self.crop_at_position = crop_at_position
        self.crop_how_many = crop_how_many
        self.is_active_insert_before = is_active_insert_before
        self.insert_before = insert_before
        self.is_active_insert_after = is_active_insert_after
        self.insert_after = insert_after
        self.is_active_insert_at_position = is_active_insert_at_position
        self.insert_at_position = insert_at_position
        self.insert_what = insert_what
        self.is_active_case_change = is_active_case_change
        self.case_change = case_change
        self.remove_accents = remove_accents
        self.remove_non_alphanumeric = remove_non_alphanumeric
        self.keep_these_non_alphanumeric = keep_these_non_alphanumeric
        self.recent_text_to_replace: list[str] = recent_text_to_replace if recent_text_to_replace is not None else []
        self.recent_text_for_replacing: list[str] = recent_text_for_replacing if recent_text_for_replacing is not None else []
        self.recent_insert_before: list[str] = recent_insert_before if recent_insert_before is not None else []
        self.recent_insert_after: list[str] = recent_insert_after if recent_insert_after is not None else []
        self.recent_insert_what: list[str] = recent_insert_what if recent_insert_what is not None else []
        
    def load_settings(self, settings: QSettings) -> None:
        p = self._prefix

        self.is_active_text_to_replace = _get_bool(settings, f"{p}is_active_text_to_replace", self.is_active_text_to_replace)
        self.text_to_replace = _get_str(settings, f"{p}text_to_replace", self.text_to_replace)
        self.text_for_replacing = _get_str(settings, f"{p}text_for_replacing", self.text_for_replacing)
        self.text_to_replace_case_sensitive = _get_bool(
            settings, f"{p}text_to_replace_case_sensitive", self.text_to_replace_case_sensitive
        )
        self.is_active_left_crop_n_characters = _get_bool(settings, f"{p}is_active_left_crop_n_characters", self.is_active_left_crop_n_characters)
        self.left_crop_n_characters = _get_int(settings, f"{p}left_crop_n_characters", self.left_crop_n_characters)
        self.is_active_right_crop_n_characters = _get_bool(settings, f"{p}is_active_right_crop_n_characters", self.is_active_right_crop_n_characters)
        self.right_crop_n_characters = _get_int(settings, f"{p}right_crop_n_characters", self.right_crop_n_characters)
        self.is_active_crop_at_position = _get_bool(settings, f"{p}is_active_crop_at_position", self.is_active_crop_at_position)
        self.crop_at_position = _get_int(settings, f"{p}crop_at_position", self.crop_at_position)
        self.crop_how_many = _get_int(settings, f"{p}crop_how_many", self.crop_how_many)
        self.is_active_insert_before = _get_bool(settings, f"{p}is_active_insert_before", self.is_active_insert_before)
        self.insert_before = _get_str(settings, f"{p}insert_before", self.insert_before)
        self.is_active_insert_after = _get_bool(settings, f"{p}is_active_insert_after", self.is_active_insert_after)
        self.insert_after = _get_str(settings, f"{p}insert_after", self.insert_after)
        self.is_active_insert_at_position = _get_bool(settings, f"{p}is_active_insert_at_position", self.is_active_insert_at_position)
        self.insert_at_position = _get_int(settings, f"{p}insert_at_position", self.insert_at_position)
        self.insert_what = _get_str(settings, f"{p}insert_what", self.insert_what)
        self.is_active_case_change = _get_bool(settings, f"{p}is_active_case_change", self.is_active_case_change)
        self.case_change = _get_enum(settings, f"{p}case_change", CaseChange, self.case_change)
        self.remove_accents = _get_bool(settings, f"{p}remove_accents", self.remove_accents)
        self.remove_non_alphanumeric = _get_bool(settings, f"{p}remove_non_alphanumeric", self.remove_non_alphanumeric)
        self.keep_these_non_alphanumeric = _get_str(settings, f"{p}keep_these_non_alphanumeric", self.keep_these_non_alphanumeric)

        self.recent_text_to_replace = _get_str_list(settings, f"{p}recent_text_to_replace", self.recent_text_to_replace)
        self.recent_text_for_replacing = _get_str_list(settings, f"{p}recent_text_for_replacing", self.recent_text_for_replacing)
        self.recent_insert_before = _get_str_list(settings, f"{p}recent_insert_before", self.recent_insert_before)
        self.recent_insert_after = _get_str_list(settings, f"{p}recent_insert_after", self.recent_insert_after)
        self.recent_insert_what = _get_str_list(settings, f"{p}recent_insert_what", self.recent_insert_what)

    # -- history helpers for text fields --

    def add_recent_text_to_replace(self, value: str) -> None:
        """Add *value* to the recent text-to-replace list (most-recent first, no duplicates)."""
        value = value.strip()
        if not value:
            return
        if value in self.recent_text_to_replace:
            self.recent_text_to_replace.remove(value)
        self.recent_text_to_replace.insert(0, value)
        self.recent_text_to_replace = self.recent_text_to_replace[:MAX_RECENT_TEXT_TO_REPLACE]

    def remove_recent_text_to_replace(self, value: str) -> None:
        """Remove *value* from the recent text-to-replace list."""
        try:
            self.recent_text_to_replace.remove(value)
        except ValueError:
            pass

    def add_recent_text_for_replacing(self, value: str) -> None:
        """Add *value* to the recent text-for-replacing list (most-recent first, no duplicates)."""
        value = value.strip()
        if not value:
            return
        if value in self.recent_text_for_replacing:
            self.recent_text_for_replacing.remove(value)
        self.recent_text_for_replacing.insert(0, value)
        self.recent_text_for_replacing = self.recent_text_for_replacing[:MAX_RECENT_TEXT_FOR_REPLACING]

    def remove_recent_text_for_replacing(self, value: str) -> None:
        """Remove *value* from the recent text-for-replacing list."""
        try:
            self.recent_text_for_replacing.remove(value)
        except ValueError:
            pass

    def add_recent_insert_before(self, value: str) -> None:
        """Add *value* to the recent insert-before list (most-recent first, no duplicates)."""
        value = value.strip()
        if not value:
            return
        if value in self.recent_insert_before:
            self.recent_insert_before.remove(value)
        self.recent_insert_before.insert(0, value)
        self.recent_insert_before = self.recent_insert_before[:MAX_RECENT_INSERT_BEFORE]

    def remove_recent_insert_before(self, value: str) -> None:
        """Remove *value* from the recent insert-before list."""
        try:
            self.recent_insert_before.remove(value)
        except ValueError:
            pass

    def add_recent_insert_after(self, value: str) -> None:
        """Add *value* to the recent insert-after list (most-recent first, no duplicates)."""
        value = value.strip()
        if not value:
            return
        if value in self.recent_insert_after:
            self.recent_insert_after.remove(value)
        self.recent_insert_after.insert(0, value)
        self.recent_insert_after = self.recent_insert_after[:MAX_RECENT_INSERT_AFTER]

    def remove_recent_insert_after(self, value: str) -> None:
        """Remove *value* from the recent insert-after list."""
        try:
            self.recent_insert_after.remove(value)
        except ValueError:
            pass

    def add_recent_insert_what(self, value: str) -> None:
        """Add *value* to the recent insert-what list (most-recent first, no duplicates)."""
        value = value.strip()
        if not value:
            return
        if value in self.recent_insert_what:
            self.recent_insert_what.remove(value)
        self.recent_insert_what.insert(0, value)
        self.recent_insert_what = self.recent_insert_what[:MAX_RECENT_INSERT_WHAT]

    def remove_recent_insert_what(self, value: str) -> None:
        """Remove *value* from the recent insert-what list."""
        try:
            self.recent_insert_what.remove(value)
        except ValueError:
            pass

    def save_settings(self, settings: QSettings) -> None:
        p = self._prefix

        settings.setValue(f"{p}is_active_text_to_replace", self.is_active_text_to_replace)
        settings.setValue(f"{p}text_to_replace", self.text_to_replace)
        settings.setValue(f"{p}text_for_replacing", self.text_for_replacing)
        settings.setValue(f"{p}text_to_replace_case_sensitive", self.text_to_replace_case_sensitive)
        settings.setValue(f"{p}is_active_left_crop_n_characters", self.is_active_left_crop_n_characters)
        settings.setValue(f"{p}left_crop_n_characters", self.left_crop_n_characters)
        settings.setValue(f"{p}is_active_right_crop_n_characters", self.is_active_right_crop_n_characters)
        settings.setValue(f"{p}right_crop_n_characters", self.right_crop_n_characters)
        settings.setValue(f"{p}is_active_crop_at_position", self.is_active_crop_at_position)
        settings.setValue(f"{p}crop_at_position", self.crop_at_position)
        settings.setValue(f"{p}crop_how_many", self.crop_how_many)
        settings.setValue(f"{p}is_active_insert_before", self.is_active_insert_before)
        settings.setValue(f"{p}insert_before", self.insert_before)
        settings.setValue(f"{p}is_active_insert_after", self.is_active_insert_after)
        settings.setValue(f"{p}insert_after", self.insert_after)
        settings.setValue(f"{p}is_active_insert_at_position", self.is_active_insert_at_position)
        settings.setValue(f"{p}insert_at_position", self.insert_at_position)
        settings.setValue(f"{p}insert_what", self.insert_what)
        settings.setValue(f"{p}is_active_case_change", self.is_active_case_change)
        settings.setValue(f"{p}case_change", int(self.case_change.value))
        settings.setValue(f"{p}remove_accents", self.remove_accents)
        settings.setValue(f"{p}remove_non_alphanumeric", self.remove_non_alphanumeric)
        settings.setValue(f"{p}keep_these_non_alphanumeric", self.keep_these_non_alphanumeric)

        _set_str_list(settings, f"{p}recent_text_to_replace", self.recent_text_to_replace)
        _set_str_list(settings, f"{p}recent_text_for_replacing", self.recent_text_for_replacing)
        _set_str_list(settings, f"{p}recent_insert_before", self.recent_insert_before)
        _set_str_list(settings, f"{p}recent_insert_after", self.recent_insert_after)
        _set_str_list(settings, f"{p}recent_insert_what", self.recent_insert_what)


class NameConfig(TextTransformConfig):
    """Configuration for renaming of the name part."""
    _prefix = "name/"


class ExtConfig(TextTransformConfig):
    """Configuration for renaming of the extension part."""
    _prefix = "ext/"


class NumberingConfig(QObject):
    # Configuration for automatic file numbering

    def __init__(self,
                 is_active_auto_number_change: bool = False,
                 auto_number_change: AutoNumberChange = AutoNumberChange.BEFORE_FILENAME,
                 start_with: int = 0,
                 increment_by: int = 1,
                 zero_fill_how_many: int = 2,
                 new_numbering_for_each_folder: bool = True):
        
        super().__init__()    

        self.is_active_auto_number_change = is_active_auto_number_change
        self.auto_number_change = auto_number_change
        self.start_with = start_with
        self.increment_by = increment_by
        self.zero_fill_how_many = zero_fill_how_many
        self.new_numbering_for_each_folder = new_numbering_for_each_folder

    def load_settings(self, settings: QSettings) -> None:

        self.is_active_auto_number_change = _get_bool(settings, "numbering/is_active_auto_number_change", self.is_active_auto_number_change)
        self.auto_number_change = _get_enum(settings, "numbering/auto_number_change", AutoNumberChange, self.auto_number_change)
        self.start_with = _get_int(settings, "numbering/start_with", self.start_with)
        self.increment_by = _get_int(settings, "numbering/increment_by", self.increment_by)
        self.zero_fill_how_many = _get_int(settings, "numbering/zero_fill_how_many", self.zero_fill_how_many)
        self.new_numbering_for_each_folder = _get_bool(settings, "numbering/new_numbering_for_each_folder", self.new_numbering_for_each_folder)

    def save_settings(self, settings: QSettings) -> None:

        settings.setValue("numbering/is_active_auto_number_change", self.is_active_auto_number_change)
        settings.setValue("numbering/auto_number_change", int(self.auto_number_change.value))
        settings.setValue("numbering/start_with", self.start_with)
        settings.setValue("numbering/increment_by", self.increment_by)
        settings.setValue("numbering/zero_fill_how_many", self.zero_fill_how_many)
        settings.setValue("numbering/new_numbering_for_each_folder", self.new_numbering_for_each_folder)


class MainConfig(QObject):

    def __init__(self):
        super().__init__()

        # Default configurations
        self.app = AppConfig()
        self.gui = GUIConfig()
        self.name = NameConfig()
        self.ext = ExtConfig()
        self.numbering = NumberingConfig()
        self.regex = RegexRenameConfig()
        self.logging = LoggingConfig()

        # Load persisted configurations
        self.settings = QSettings()
        self.load_settings()

    def load_settings(self) -> None:
        """Load settings persisted between application runs."""

        self.app.load_settings(self.settings)
        self.gui.load_settings(self.settings)
        self.name.load_settings(self.settings)
        self.ext.load_settings(self.settings)
        self.numbering.load_settings(self.settings)
        self.regex.load_settings(self.settings)
        self.logging.load_settings(self.settings)
        logging.getLogger("filename_ninja.config").debug("Settings loaded from %s", self.settings.fileName())

    def save_settings(self) -> None:
        """Persist settings between application runs."""

        self.app.save_settings(self.settings)
        self.gui.save_settings(self.settings)
        self.name.save_settings(self.settings)
        self.ext.save_settings(self.settings)
        self.numbering.save_settings(self.settings)
        self.regex.save_settings(self.settings)
        self.logging.save_settings(self.settings)
        logging.getLogger("filename_ninja.config").debug("Settings saved to %s", self.settings.fileName())
