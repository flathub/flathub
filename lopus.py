#!/usr/bin/env python3
"""Lopus - a dual-pane file manager for Linux."""

import hashlib
import os
import shutil
import stat as statmod
import subprocess
import sys
import tarfile
import time
import zipfile
from datetime import datetime

from PyQt6.QtCore import QFileInfo, QFileSystemWatcher, QPointF, QMimeData, QSize, Qt, QTimer, QThread, pyqtSignal
from PyQt6.QtGui import QAction, QColor, QDrag, QFontDatabase, QIcon, QPainter, QPixmap
from PyQt6.QtWidgets import (
    QAbstractItemView,
    QApplication,
    QColorDialog,
    QCheckBox,
    QComboBox,
    QFileDialog,
    QGridLayout,
    QListWidget,
    QListWidgetItem,
    QDialog,
    QDialogButtonBox,
    QDockWidget,
    QFileIconProvider,
    QFontComboBox,
    QHBoxLayout,
    QHeaderView,
    QInputDialog,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMenu,
    QMessageBox,
    QPlainTextEdit,
    QProgressBar,
    QPushButton,
    QSpinBox,
    QSplitter,
    QSizePolicy,
    QStackedWidget,
    QTabBar,
    QTabWidget,
    QTableWidget,
    QTableWidgetItem,
    QAbstractItemView,
    QToolBar,
    QToolButton,
    QTreeWidget,
    QTreeWidgetItem,
    QVBoxLayout,
    QWidget,
)

# Taal instellen VÓÓR het importeren van modules die tr() al op
# module-niveau evalueren (zoals de COLUMNS/BULK_FIELDS-tabellen in
# tageditor.py). Anders "bevriezen" die titels in het Nederlands.
try:
    import json as _json
    _cfg_l = os.path.join(
        os.environ.get("LOPUS_CONFIG_DIR")
        or os.path.expanduser("~/.config/lopus"), "tabs.json")
    with open(_cfg_l, encoding="utf-8") as _f:
        _lang = _json.load(_f).get("language")
    if _lang in ("nl", "en"):
        from translations import set_language as _set_lang
        _set_lang(_lang)
except Exception:
    pass

import json
import re

# --- Module imports met veilige fallbacks ---
try:
    from translations import tr, set_language
except ImportError:
    def tr(text): return text
    def set_language(lang): pass

try:
    import mutagen  # optioneel: nodig voor de tag-editor (pip install mutagen)
except ImportError:
    mutagen = None

try:
    from tageditor import TagEditor
except ImportError:
    TagEditor = None

try:
    from renamer import Renamer
except ImportError:
    Renamer = None

try:
    from ftp import FtpDialog
except ImportError:
    FtpDialog = None


APP_NAME = "Lopus"
COL_NAME, COL_SIZE, COL_TYPE, COL_MODIFIED = 0, 1, 2, 3
# Eénmalige migratie: instellingen uit de oude dopus-map overnemen
_OLD_CONFIG_DIR = os.path.expanduser("~/.config/dopus")
# Testen (en speciale gevallen) kunnen een eigen configuratiemap forceren
# via LOPUS_CONFIG_DIR, zodat ze nooit de echte gebruikersinstellingen of
# echte bestanden raken.
_CONFIG_DIR = os.environ.get("LOPUS_CONFIG_DIR") or os.path.expanduser(
    "~/.config/lopus")
if not os.environ.get("LOPUS_CONFIG_DIR") and os.path.isdir(_OLD_CONFIG_DIR) \
        and not os.path.isdir(_CONFIG_DIR):
    try:
        import shutil as _shutil
        _shutil.copytree(
            _OLD_CONFIG_DIR, _CONFIG_DIR,
            ignore=_shutil.ignore_patterns("error.log"))
    except OSError:
        pass  # migratie mislukt? dan gewoon met schone map starten
CONFIG_PATH = os.path.join(_CONFIG_DIR, "tabs.json")
ERROR_LOG = os.path.join(_CONFIG_DIR, "error.log")

ICON_PROVIDER = QFileIconProvider()


def log_error(msg, exc=None):
    """Write errors to console and to ~/.config/lopus/error.log."""
    import traceback

    line = f"[{datetime.now():%d-%m-%Y %H:%M:%S}] {msg}"
    if exc is not None:
        line += "\n" + "".join(
            traceback.format_exception(type(exc), exc, exc.__traceback__)
        )
    print(line, file=sys.stderr, flush=True)
    try:
        os.makedirs(os.path.dirname(ERROR_LOG), exist_ok=True)
        with open(ERROR_LOG, "a") as f:
            f.write(line + "\n\n")
    except OSError:
        pass


def human_size(n):
    if n is None:
        return ""
    for unit in ("B", "KB", "MB", "GB", "TB"):
        if abs(n) < 1024:
            return f"{n:.0f} {unit}" if unit == "B" else f"{n:.1f} {unit}"
        n /= 1024.0
    return f"{n:.1f} PB"


def load_ui_settings():
    """Read the UI settings json, returning {} on any problem."""
    try:
        with open(CONFIG_PATH) as f:
            data = json.load(f)
        return data if isinstance(data, dict) else {}
    except (OSError, ValueError):
        return {}


# ---------- theme ----------
DEFAULT_THEME = {
    "font_family": "",        # empty = system default (file panes only)
    "font_size": 0,           # 0 = system default
    "color_text": "",         # text color everywhere outside panes
    "color_bg_window": "",    # background color outside panes
    "color_text_pane": "",    # text color in file lists
    "color_bg_pane": "",      # background color of file lists
    "color_bg_nav": "",       # background color of navigation bar
    "color_nav_text": "",     # text color of navigation bar
    "size_functiebalk": 0,    # 0 = automatic
    "size_knoppenbalk": 0,
    "size_tabbladstrook": 0,
    "size_navigatiebalk": 0,
    "color_tab_selected": "",  # background of the active tab
    "color_tab_selected_text": "",  # text color of the active tab
    "color_menu_bg": "",       # background of (context) menus
    "color_menu_text": "",     # text color of menu items
    "color_menu_sel": "",      # highlight color of the hovered/selected item
    "color_menu_sep": "",      # color of the separator lines in menus
    "color_file_afbeelding": "",   # bestandsnaam-kleur afbeeldingen
    "color_file_video": "",        # bestandsnaam-kleur video
    "color_file_audio": "",        # bestandsnaam-kleur audio
    "color_file_archief": "",      # bestandsnaam-kleur archieven
    "color_file_document": "",     # bestandsnaam-kleur documenten
}

# Kleurcodering van bestanden op type (aan/uit via weergave-knop)
FILE_CATEGORIES = [
    ("afbeelding",
     {".png", ".jpg", ".jpeg", ".gif", ".bmp", ".svg", ".webp", ".tiff",
      ".tif", ".ico", ".heic", ".xcf"}, "#3f8fd2"),
    ("video",
     {".mp4", ".mkv", ".avi", ".mov", ".webm", ".wmv", ".m4v", ".mpg",
      ".mpeg", ".ts"}, "#a05fc9"),
    ("audio",
     {".mp3", ".flac", ".ogg", ".oga", ".opus", ".m4a", ".m4b", ".wav",
      ".wma", ".aac"}, "#2fae6b"),
    ("archief",
     {".zip", ".tar", ".gz", ".bz2", ".xz", ".7z", ".rar", ".zst", ".tgz"},
     "#d2843f"),
    ("document",
     {".pdf", ".doc", ".docx", ".odt", ".xls", ".xlsx", ".ods", ".ppt",
      ".pptx", ".txt", ".md", ".rtf", ".epub"}, "#c9b23f"),
]


def file_category(ext):
    """Categorie-naam voor een extensie (met punt, lowercase) of None."""
    for cat, exts, _default in FILE_CATEGORIES:
        if ext in exts:
            return cat
    return None


def category_colors(theme=None):
    """{categorie: QColor} — kleur uit het thema of de ingebouwde default."""
    if theme is None:
        theme = current_theme()
    out = {}
    for cat, _exts, default in FILE_CATEGORIES:
        val = theme.get(f"color_file_{cat}") or default
        out[cat] = QColor(val)
    return out


# ---------- scripting-interface (eigen opdrachten in Python) ----------
SCRIPTS_DIR = os.path.expanduser("~/.config/lopus/scripts")

EXAMPLE_SCRIPT = '''"""Voorbeeld-opdracht voor Lopus.

Een script definieert een functie run(main, panel):
  main  = het hoofdvenster (MainWindow)
  panel = het actieve paneel (FilePanel)

Handige dingen:
  panel.current_path              -> huidige map
  panel.selected_paths()          -> geselecteerde bestanden (volledige paden)
  panel.refresh()                 -> lijst verversen
  main.set_status("tekst", 3000)  -> melding in de statusbalk
  main.focused_panel()            -> het paneel dat focus heeft
  main.left / main.right          -> linker-/rechterpaneel
  main._run_transfer(jobs, move)  -> kopieren/verplaatsen met voortgang
  lopus-functies zijn beschikbaar via: import lopus

Zet TOOLBAR = True voor ook een knop in de knoppenbalk.
"""

TITLE = "Voorbeeld: toon selectie"
TOOLBAR = False


def run(main, panel):
    paths = panel.selected_paths()
    if paths:
        main.set_status(f"Geselecteerd: {len(paths)} item(s), "
                        f"eerste: {paths[0]}", 6000)
    else:
        main.set_status(f"Huidige map: {panel.current_path}", 6000)
'''


def ensure_scripts_dir():
    """Maak de scripts-map aan (met een voorbeeldscript als die leeg is)."""
    try:
        os.makedirs(SCRIPTS_DIR, exist_ok=True)
        if not [f for f in os.listdir(SCRIPTS_DIR) if f.endswith(".py")]:
            with open(os.path.join(SCRIPTS_DIR, "voorbeeld.py"), "w") as f:
                f.write(EXAMPLE_SCRIPT)
    except OSError as e:
        log_error("scripts-map aanmaken faalde", e)


def load_user_scripts():
    """Laad alle .py-scripts uit ~/.config/lopus/scripts.

    Een script moet een functie run(main, panel) definiëren; optionele
    variabelen: TITLE (menunaam), TOOLTIP, TOOLBAR (True = ook knop)."""
    ensure_scripts_dir()
    import importlib.util
    scripts = []
    try:
        files = sorted(os.listdir(SCRIPTS_DIR))
    except OSError:
        return scripts
    for fn in files:
        if not fn.endswith(".py") or fn.startswith("_"):
            continue
        path = os.path.join(SCRIPTS_DIR, fn)
        try:
            spec = importlib.util.spec_from_file_location(
                f"lopus_script_{fn[:-3]}", path)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            if hasattr(mod, "run"):
                scripts.append({
                    "name": str(getattr(mod, "TITLE", fn[:-3])),
                    "tooltip": str(getattr(mod, "TOOLTIP", fn)),
                    "toolbar": bool(getattr(mod, "TOOLBAR", False)),
                    "module": mod,
                    "file": path,
                })
        except Exception as e:  # noqa: BLE001
            log_error(f"script {fn} kon niet geladen worden", e)
    return scripts


def _menu_check_image(color):
    """Genereer een klein vinkje als PNG (voor QMenu::indicator:checked),
    omdat Qt bij gestylede menu's het native vinkje niet meer tekent."""
    path = os.path.join(os.path.dirname(CONFIG_PATH), "menu_check.png")
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        pm = QPixmap(14, 14)
        pm.fill(Qt.GlobalColor.transparent)
        p = QPainter(pm)
        pen = p.pen()
        pen.setColor(QColor(color))
        pen.setWidth(2)
        p.setPen(pen)
        p.drawPolyline([QPointF(2, 7), QPointF(6, 11), QPointF(12, 3)])
        p.end()
        pm.save(path, "PNG")
        return path
    except Exception:  # noqa: BLE001
        return None


def apply_theme(theme):
    """Apply fonts and colors app-wide via a stylesheet."""
    parts = []
    # Tabs: horizontal breathing room around the label
    parts.append("QTabBar::tab { padding-left: 10px; padding-right: 10px; }")
    sel_tab = theme.get("color_tab_selected") or ""
    sel_tab_text = theme.get("color_tab_selected_text") or ""
    if sel_tab or sel_tab_text:
        bits = []
        if sel_tab:
            bits.append(f"background-color: {sel_tab};")
        if sel_tab_text:
            bits.append(f"color: {sel_tab_text};")
        parts.append("QTabBar::tab:selected { " + " ".join(bits) + " }")
    # Font applies ONLY to file lists (trees), not menus/toolbars
    font_bits = []
    if theme.get("font_family"):
        font_bits.append(f"font-family: '{theme['font_family']}';")
    if theme.get("font_size"):
        try:
            font_bits.append(f"font-size: {int(theme['font_size'])}pt;")
        except (TypeError, ValueError):
            pass
    if theme.get("color_bg_window") or theme.get("color_text"):
        w = []
        if theme.get("color_bg_window"):
            w.append(f"background-color: {theme['color_bg_window']};")
        if theme.get("color_text"):
            w.append(f"color: {theme['color_text']};")
        parts.append("QMainWindow, QDialog, QMenuBar, QMenu, QToolBar, "
                     + "QStatusBar { " + " ".join(w) + " }")
    if theme.get("color_bg_pane") or theme.get("color_text_pane") or font_bits:
        p = list(font_bits)
        bg = theme.get("color_bg_pane") or ""
        if bg:
            p.append(f"background-color: {bg};")
            # Same colour for odd/even rows: no zebra striping
            p.append(f"alternate-background-color: {bg};")
            p.append("QTreeWidget::item { border: none; }")
        if theme.get("color_text_pane"):
            p.append(f"color: {theme['color_text_pane']};")
        p.append("selection-background-color: #3a5a8c; selection-color: white;")
        parts.append("QTreeWidget { " + " ".join(p) + " }")
    if theme.get("color_bg_nav") or theme.get("color_nav_text"):
        bg = theme.get("color_bg_nav") or ""
        fg = theme.get("color_nav_text") or ""
        if bg:
            parts.append(f"QWidget#navRow {{ background-color: {bg}; }}")
            parts.append(
                f"QWidget#navRow QLineEdit, QWidget#navRow QPushButton "
                f"{{ background-color: {bg}; color: {fg or 'palette(text)'}; }}"
            )
        elif fg:
            parts.append(
                f"QWidget#navRow QLineEdit, QWidget#navRow QPushButton "
                f"{{ color: {fg}; }}"
            )

    # ---- (context)menu's: achtergrond, tekst, selectie en separators ----
    m_bg = theme.get("color_menu_bg") or ""
    m_text = theme.get("color_menu_text") or ""
    m_sel = theme.get("color_menu_sel") or ""
    m_sep = theme.get("color_menu_sep") or ""
    if m_bg or m_text or m_sel or m_sep:
        menu_css = ["QMenu { padding: 3px;"]
        if m_bg:
            menu_css.append(f"background-color: {m_bg}; border: 1px solid {m_bg};")
        if m_text:
            menu_css.append(f"color: {m_text};")
        menu_css.append("}")
        menu_css.append("QMenu::item { padding: 4px 22px 4px 24px; background: transparent; }")
        if m_text:
            menu_css.append(f"QMenu::item {{ color: {m_text}; }}")
        if m_sel:
            menu_css.append(
                f"QMenu::item:selected {{ background-color: {m_sel}; "
                f"color: {m_bg or 'white'}; }}")
        # Vinkjes (aangevinkte menu-items) zichtbaar houden: Qt tekent het
        # native vinkje niet meer zodra QMenu::item gestyled is.
        check_img = _menu_check_image(m_text or "#ffffff")
        menu_css.append(
            "QMenu::indicator { width: 14px; height: 14px; left: 5px; }")
        if check_img:
            menu_css.append(
                f"QMenu::indicator:checked {{ image: url({check_img}); }}")
        if m_sep:
            menu_css.append(
                f"QMenu::separator {{ height: 1px; background: {m_sep}; "
                f"margin: 4px 8px; }}")
        parts.append(" ".join(menu_css))

    # ---- bar sizes (heights in px; 0/absent = automatic) ----
    def _h(key):
        try:
            return int(theme.get(key) or 0)
        except (TypeError, ValueError):
            return 0

    h_func = _h("size_functiebalk")
    h_btn = _h("size_knoppenbalk")
    h_tab = _h("size_tabbladstrook")
    h_nav = _h("size_navigatiebalk")

    if h_func > 0:
        parts.append(
            f"QToolBar#FunctionToolBar {{ min-height: {h_func}px; "
            f"max-height: {h_func}px; padding: 0px; margin: 0px; spacing: 2px; }}"
            f"QToolBar#FunctionToolBar QToolButton {{ padding: 0px 3px; margin: 0px; }}"
        )
    if h_btn > 0:
        parts.append(
            f"QToolBar#ButtonBar {{ min-height: {h_btn}px; "
            f"max-height: {h_btn}px; padding: 0px; margin: 0px; }}"
            f"QToolBar#ButtonBar QToolButton {{ padding: 0px 4px; margin: 0px; }}"
        )
    if h_tab > 0:
        # Only a minimum height: a max-height makes Qt elide/clipped labels
        parts.append(
            f"QTabBar::tab {{ min-height: {h_tab}px; "
            f"padding-top: 0px; padding-bottom: 0px; }}"
        )
    if h_nav > 0:
        parts.append(
            f"QWidget#navRow {{ min-height: {h_nav}px; max-height: {h_nav}px; "
            f"padding: 0px; margin: 0px; }}"
            "QWidget#navRow QPushButton#navBtn { min-width: 18px; "
            "max-width: 26px; padding: 0px; margin: 0px; }"
            "QWidget#navRow QPushButton#crumbBtn { padding: 0px 3px; "
            "margin: 0px; }"
        )
    QApplication.instance().setStyleSheet("\n".join(parts))


def bar_icon_size(theme, key, default=18):
    """Icon size derived from a configured bar height."""
    try:
        h = int(theme.get(key) or 0)
    except (TypeError, ValueError):
        h = 0
    s = (h - 8) if h > 0 else default
    return max(8, min(s, default))


def current_theme():
    t = dict(DEFAULT_THEME)
    t.update({k: v for k, v in load_ui_settings().items() if k in DEFAULT_THEME})
    return t


# ---------- disk image mounting ----------
MOUNTABLE_EXTS = {".iso", ".mdf", ".nrg", ".img", ".bin"}
_mounted_images = {}  # image path -> {"device": /dev/loopN, "mountpoint": ...}


def _run_cmd(args, timeout=30):
    try:
        return subprocess.run(
            args, capture_output=True, text=True, timeout=timeout
        )
    except (OSError, subprocess.TimeoutExpired):
        return None


def mount_disk_image(path):
    """Mount an ISO/MDF/... via udisksctl (no root needed).

    Returns (mountpoint_or_None, error_or_None).
    """
    if not shutil.which("udisksctl"):
        return None, "udisksctl niet gevonden (installeer udisks2)"
    r = _run_cmd(["udisksctl", "loop-setup", "-f", path])
    if r is None or r.returncode != 0:
        err = (r.stderr.strip() if r else "onbekende fout") or "onbekende fout"
        return None, err
    import re

    m = re.search(r"/dev/loop\d+", r.stdout)
    if not m:
        return None, "Kon loop-apparaat niet bepalen:\n" + r.stdout
    dev = m.group(0)
    r2 = _run_cmd(["udisksctl", "mount", "-b", dev])
    mp = None
    if r2 is not None:
        m2 = re.search(r"at (.+)$", r2.stdout.strip(), re.MULTILINE)
        if m2:
            mp = m2.group(1).strip()
    if not mp:
        # Try lsblk as fallback
        r3 = _run_cmd(["lsblk", "-no", "MOUNTPOINT", dev])
        if r3 is not None and r3.stdout.strip():
            mp = r3.stdout.strip().splitlines()[0]
    if not mp:
        _run_cmd(["udisksctl", "loop-delete", "-b", dev])
        return None, "Kon apparaat niet aankoppelen"
    _mounted_images[path] = {"device": dev, "mountpoint": mp}
    return mp, None


def unmount_disk_image(path):
    """Unmount a previously mounted image. Returns error or None."""
    info = _mounted_images.pop(path, None)
    if info is None:
        return "Deze image is niet via Lopus gekoppeld"
    r = _run_cmd(["udisksctl", "unmount", "-b", info["device"]])
    if r is None or r.returncode != 0:
        return (r.stderr.strip() if r else "onbekende fout") or "onbekende fout"
    _run_cmd(["udisksctl", "loop-delete", "-b", info["device"]])
    return None


def trash_paths(paths):
    """Move paths to the trash using gio. Returns list of (path, error)."""
    failed = []
    for p in paths:
        try:
            r = subprocess.run(
                ["gio", "trash", p], capture_output=True, text=True, timeout=30
            )
            if r.returncode != 0:
                failed.append((p, r.stderr.strip() or "onbekende fout"))
        except (OSError, subprocess.TimeoutExpired) as e:
            failed.append((p, str(e)))
    return failed


# ---------- netwerk-/FUSE-mounts (rclone, nfs, cifs, sshfs, ...) ----------
_REMOTE_CACHE = {"ts": 0.0, "mounts": []}
# Bewust géén generieke "fuse"/"fuseblk": lokale schijven (bijv. NTFS via
# ntfs-3g) worden ook als fuseblk gemount en zijn geen netwerk.
_REMOTE_FSTYPES = ("rclone", "nfs", "cifs", "smbfs", "sshfs",
                   "davfs", "curlftpfs", "ncpfs", "9p")


def remote_mount_details():
    """(mountpunt, fstype, apparaat) van netwerk-/FUSE-bestandssystemen
    (uit /proc/mounts, 2 s gecached)."""
    now = time.monotonic()
    if now - _REMOTE_CACHE["ts"] > 2.0:
        mounts = []
        try:
            with open("/proc/mounts") as f:
                for line in f:
                    parts = line.split()
                    if len(parts) < 3:
                        continue
                    dev, mp, fstype = parts[0], parts[1], parts[2]
                    mp = mp.replace("\\040", " ")
                    # Pseudo-filesystems (zoals nfsd van de server-dienst)
                    # zijn geen shares en mogen nooit als netwerk tellen.
                    if (mp.startswith(("/proc", "/sys", "/dev"))
                            or fstype == "nfsd"):
                        continue
                    if (fstype.startswith(_REMOTE_FSTYPES)
                            or "rclone" in dev.lower()):
                        if mp != "/":
                            mounts.append((mp, fstype, dev))
                    elif fstype.startswith("fuse"):
                        # gvfs-fuse: alleen de netwerkshares eronder (smb,
                        # nfs, ...) tellen als netwerk, niet de mount zelf.
                        # fuse.rclone (OneDrive e.d.) is altijd netwerk.
                        low = mp.lower()
                        if ("gvfs" in low or "rclone" in low
                                or "rclone" in fstype.lower()
                                or ":" in mp or "smb" in low):
                            if mp != "/":
                                mounts.append((mp, fstype, dev))
        except OSError:
            pass
        _REMOTE_CACHE["ts"] = now
        _REMOTE_CACHE["mounts"] = mounts
    return _REMOTE_CACHE["mounts"]


def remote_mounts():
    """Mountpunten van netwerk-/FUSE-bestandssystemen (uit /proc/mounts)."""
    return [mp for mp, _fstype, _dev in remote_mount_details()]


def mount_host(mp, fstype, dev):
    """Probeer de host (IP/computernaam) van een netwerk-mount te bepalen.
    Geeft None als dat niet lukt."""
    low = mp.lower()
    if fstype.startswith("nfs") and ":" in dev:
        return dev.split(":", 1)[0]
    if fstype in ("cifs", "smbfs") and dev.startswith("//"):
        return dev[2:].split("/", 1)[0]
    if "sshfs" in fstype and "@" in dev:
        tail = dev.split("@", 1)[1]
        return tail.split(":", 1)[0] or None
    # rclone-mounts (OneDrive e.d.): geen echte host; groepeer onder rclone
    if "rclone" in fstype.lower() or "rclone" in low or "rclone" in dev.lower():
        return "rclone"
    # gvfs-koppelpunten: smb-share:server=NAME,share=NAME / nfs-mount:...
    if "server=" in low:
        try:
            seg = low.split("server=", 1)[1].split(",", 1)[0].strip()
            if seg:
                return seg
        except Exception:  # noqa: BLE001
            pass
    return None


def is_remote_path(path):
    """True als pad op een netwerk-/FUSE-mount ligt (bijv. rclone OneDrive)."""
    p = os.path.abspath(path)
    for mp in remote_mounts():
        if p == mp or p.startswith(mp.rstrip("/") + "/"):
            return True
    return False


_ALL_MOUNTS_CACHE = {"ts": 0.0, "mounts": []}


def all_mounts():
    """Alle mountpunten uit /proc/mounts als [(mountpunt, apparaat)],
    langste pad eerst (2 s gecached)."""
    now = time.monotonic()
    if now - _ALL_MOUNTS_CACHE["ts"] > 2.0:
        mounts = []
        try:
            with open("/proc/mounts") as f:
                for line in f:
                    parts = line.split()
                    if len(parts) < 3:
                        continue
                    mp = parts[1].replace("\\040", " ")
                    mounts.append((mp, parts[0]))
        except OSError:
            pass
        mounts.sort(key=lambda m: -len(m[0]))
        _ALL_MOUNTS_CACHE["ts"] = now
        _ALL_MOUNTS_CACHE["mounts"] = mounts
    return _ALL_MOUNTS_CACHE["mounts"]


def mountpoint_of(path):
    """Het mountpoint waarin 'path' ligt (langste match, dus niet altijd /)."""
    p = os.path.abspath(path)
    for mp, _dev in all_mounts():
        if p == mp or p.startswith(mp.rstrip("/") + "/"):
            return mp
    return "/"


def scan_dir_entries(path, show_hidden=True):
    """Map-inhoud lezen (kan lang duren op netwerk-mounts).
    Gebruikt os.scandir: de stat-gegevens die het bestandssysteem bij het
    lezen van de map al aanlevert worden hergebruikt — op netwerk-mounts
    (rclone e.d.) voorkomt dit een extra netwerkactie PER BESTAND."""
    folders, files = [], []
    try:
        with os.scandir(path) as it:
            entries = list(it)
    except OSError:
        return folders, files
    for e in sorted(entries, key=lambda e: e.name.lower()):
        name = e.name
        if name.startswith(".") and not show_hidden:
            continue
        full = e.path
        try:
            st = e.stat(follow_symlinks=False)
        except OSError:
            continue
        entry = {
            "name": name,
            "full": full,
            "isdir": statmod.S_ISDIR(st.st_mode),
            "islink": e.is_symlink(),
            "size": None if statmod.S_ISDIR(st.st_mode) else st.st_size,
            "mtime": st.st_mtime,
        }
        (folders if entry["isdir"] else files).append(entry)
    return folders, files


class DirLoader(QThread):
    """Leest mapinhoud op de achtergrond (voor trage netwerk-mounts)."""
    scanned = pyqtSignal(str, object, object)  # path, folders|None, files|None

    def __init__(self, path, show_hidden, parent=None):
        super().__init__(parent)
        self.path = path
        self.show_hidden = show_hidden

    def run(self):
        # isdir kan op een dode/trage netwerk-mount lang duren: daarom hier,
        # in de thread, en niet in de UI-thread.
        try:
            if not os.path.isdir(self.path):
                self.scanned.emit(self.path, None, None)
                return
        except OSError:
            self.scanned.emit(self.path, None, None)
            return
        folders, files = scan_dir_entries(self.path, self.show_hidden)
        self.scanned.emit(self.path, folders, files)


class FuncThread(QThread):
    """Voert een functie op de achtergrond uit; resultaat via done-signaal."""
    done = pyqtSignal(object)

    def __init__(self, fn, parent=None):
        super().__init__(parent)
        self.fn = fn

    def run(self):
        try:
            result = self.fn()
        except Exception as e:  # noqa: BLE001
            result = e
        self.done.emit(result)


def _root_gui_env():
    """Omgevingsvariabelen die een root-GUI-programma nodig heeft."""
    keys = ("DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY",
            "XDG_RUNTIME_DIR", "DBUS_SESSION_BUS_ADDRESS", "HOME")
    return [f"{k}={os.environ[k]}" for k in keys if os.environ.get(k)]


def open_root_lopus(path=None):
    """Start een tweede Lopus als root (pkexec), eventueel op een map.

    Returnt None bij succes of een foutmelding."""
    if shutil.which("pkexec") is None:
        return "pkexec is niet geïnstalleerd"
    script = os.path.abspath(__file__)
    cmd = ["pkexec", "env"] + _root_gui_env() + [sys.executable, script]
    if path and os.path.isdir(path):
        cmd.append(path)
    try:
        subprocess.Popen(cmd, cwd=os.path.expanduser("~"),
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
        return None
    except OSError as e:
        return str(e)


def open_file_as_root(path):
    """Open één bestand in de standaard-applicatie als root (pkexec)."""
    if shutil.which("pkexec") is None:
        return "pkexec is niet geïnstalleerd"
    try:
        subprocess.Popen(
            ["pkexec", "env"] + _root_gui_env() + ["xdg-open", path],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return None
    except OSError as e:
        return str(e)


def get_mime_type(path):
    """Return mime type string for path."""
    try:
        r = subprocess.run(
            ["xdg-mime", "query", "filetype", path],
            capture_output=True, text=True, timeout=10,
        )
        if r.returncode == 0 and r.stdout.strip():
            return r.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        pass
    import mimetypes
    return mimetypes.guess_type(path)[0] or "application/octet-stream"


def list_apps_for_mime(mime):
    """Return (default_desktop_id_or_None, [desktop_id, ...])."""
    try:
        r = subprocess.run(
            ["gio", "mime", mime], capture_output=True, text=True, timeout=10
        )
        lines = r.stdout.splitlines()
    except (OSError, subprocess.TimeoutExpired):
        return None, []
    default = None
    apps = []
    section = None
    for line in lines:
        s = line.strip()
        if not s:
            continue
        low = s.lower()
        if "standaard" in low or "default" in low:
            section = "default"
            # e.g. "Standaardapplicatie voor 'text/plain': foo.desktop"
            if ":" in s:
                tail = s.rsplit(":", 1)[1].strip()
                if tail and not tail.startswith(("geen", "none")):
                    default = tail
                    section = None
            continue
        if "geregistreerd" in low or "registered" in low or "recommended" in low:
            section = "apps"
            continue
        if s.endswith(".desktop"):
            if section == "default" and default is None:
                default = s
            elif s not in apps:
                apps.append(s)
    if default in apps:
        apps.remove(default)
    return default, apps


def find_desktop_file(desktop_id):
    """Locate a .desktop file by id; returns path or None."""
    xdg_data = os.environ.get("XDG_DATA_DIRS", "/usr/local/share:/usr/share")
    dirs = [os.path.expanduser("~/.local/share/applications"),
            os.path.expanduser("~/.local/share/flatpak/exports/share/applications")]
    dirs += [os.path.join(d, "applications") for d in xdg_data.split(":")]
    subdirs = ["", "applications"]
    for base in dirs:
        for sub in subdirs:
            p = os.path.join(base, desktop_id)
            if os.path.isfile(p):
                return p
    return None


def desktop_app_name(desktop_id):
    """Human readable name from a .desktop file, fallback to id."""
    path = find_desktop_file(desktop_id)
    if path:
        try:
            with open(path) as f:
                for line in f:
                    line = line.strip()
                    if line.startswith("Name=") and line[5:].strip():
                        return line[5:].strip()
        except OSError:
            pass
    return desktop_id.removesuffix(".desktop")


def _desktop_exec_argv(dpath, paths):
    """Convert a .desktop Exec= line into an argv list with paths inserted."""
    import re
    import shlex

    exec_line = None
    try:
        with open(dpath) as f:
            for line in f:
                line = line.strip()
                if line.startswith("Exec="):
                    exec_line = line[5:]
                    break
    except OSError:
        return None
    if not exec_line:
        return None
    try:
        tokens = shlex.split(exec_line)
    except ValueError:
        return None
    argv = []
    inserted = False
    for tok in tokens:
        if tok.startswith("%"):
            if re.search(r"%[fFuU]", tok) and not inserted:
                argv.extend(paths)
                inserted = True
            continue  # skip other field codes (%k, %i, %c, ...)
        argv.append(tok)
    if not inserted:
        argv.extend(paths)
    return argv


def open_with_app(desktop_id, paths):
    """Open paths with the application identified by desktop_id."""
    dpath = find_desktop_file(desktop_id)
    if dpath:
        argv = _desktop_exec_argv(dpath, paths)
        if argv:
            try:
                subprocess.Popen(argv)
                return True
            except OSError:
                pass
        # Fallback: gio launch (paths as URIs)
        try:
            uris = ["file://" + p if not p.startswith("/") else p for p in paths]
            subprocess.Popen(["gio", "launch", dpath] + uris)
            return True
        except OSError:
            pass
    # Last resort: let the OS decide via xdg-open
    try:
        subprocess.Popen(["xdg-open"] + list(paths))
        return True
    except OSError:
        return False


def set_default_app(mime, desktop_id):
    """Persist desktop_id as default handler for mime."""
    r = subprocess.run(
        ["xdg-mime", "default", desktop_id, mime],
        capture_output=True, text=True, timeout=10,
    )
    return r.returncode == 0


def new_empty_file(directory):
    """Ask for a name and create an empty file in directory."""
    name, ok = QInputDialog.getText(
        None, "Nieuw bestand",
        "Bestandsnaam (bijv. test.txt of test.yaml):",
        text="nieuwbestand.txt",
    )
    if ok and name.strip():
        name = name.strip()
        target = os.path.join(directory, name)
        if os.path.exists(target):
            QMessageBox.warning(None, APP_NAME, f"Bestaat al:\n{target}")
            return False
        try:
            open(target, "w").close()
            return True
        except OSError as e:
            QMessageBox.warning(None, APP_NAME, str(e))
    return False


def unique_path(p):
    """Return p, or p with a ' (n)' suffix if it already exists."""
    if not os.path.exists(p):
        return p
    d, n = os.path.split(p)
    base, ext = os.path.splitext(n)
    i = 1
    while True:
        cand = os.path.join(d, f"{base} ({i}){ext}")
        if not os.path.exists(cand):
            return cand
        i += 1


# ---------- terminal ----------
def open_terminal_in(path):
    """Open a terminal window in `path`. Returns error or None."""
    path = path if os.path.isdir(path) else os.path.dirname(path)
    candidates = [
        ("x-terminal-emulator", ["--working-directory={path}"]),
        ("gnome-terminal", ["--working-directory={path}"]),
        ("kgx", ["--working-directory={path}"]),
        ("konsole", ["--workdir", "{path}"]),
        ("xfce4-terminal", ["--working-directory={path}"]),
        ("alacritty", ["--working-directory", "{path}"]),
        ("kitty", ["--directory", "{path}"]),
    ]
    for name, tmpl in candidates:
        exe = shutil.which(name)
        if not exe:
            continue
        args = [a.replace("{path}", path) for a in tmpl]
        try:
            subprocess.Popen([exe] + args)
            return None
        except OSError as e:
            return str(e)
    return "Geen terminal gevonden (installeer bijv. gnome-terminal)"


def sha256_of(path):
    """Chunked SHA-256 hex digest of a file."""
    import hashlib

    h = hashlib.sha256()
    try:
        with open(path, "rb") as f:
            while True:
                chunk = f.read(1024 * 1024)
                if not chunk:
                    break
                h.update(chunk)
        return h.hexdigest()
    except OSError as e:
        return f"FOUT: {e}"


class DirSizeWorker(QThread):
    size_ready = pyqtSignal(str, int)

    def __init__(self, paths, parent=None):
        super().__init__(parent)
        self.paths = paths

    def run(self):
        for p in self.paths:
            total = 0
            try:
                for root, _d, files in os.walk(p):
                    for f in files:
                        try:
                            sz = os.path.getsize(os.path.join(root, f))
                            if sz > 0:  # ignore weird/negative reports
                                total += sz
                        except OSError:
                            continue
            except OSError:
                continue
            self.size_ready.emit(p, max(total, 0))


class FuncWorker(QThread):
    """Run a callable in a background thread; emits its return value."""

    result_ready = pyqtSignal(object)

    def __init__(self, fn, parent=None):
        super().__init__(parent)
        self.fn = fn

    def run(self):
        try:
            res = self.fn()
        except Exception as e:  # noqa: BLE001
            res = f"FOUT: {e}"
        self.result_ready.emit(res)


# ---------- properties / executable / sharing ----------
def _file_type_label(path, st):
    if statmod.S_ISDIR(st.st_mode):
        return "Map"
    if statmod.S_ISLNK(st.st_mode):
        return "Snelkoppeling"
    return get_mime_type(path)


def show_properties_dialog(parent, path):
    """Build and show a properties dialog for path. Returns None."""
    try:
        st = os.stat(path)
        lst = os.lstat(path)
    except OSError as e:
        QMessageBox.warning(parent, "Eigenschappen", str(e))
        return
    import grp
    import pwd

    is_dir = statmod.S_ISDIR(lst.st_mode)
    size_txt = "<map>" if is_dir else human_size(st.st_size)
    extra = ""
    if is_dir:
        n_files = n_dirs = 0
        total_size = 0
        for root, dirs, files in os.walk(path):
            n_files += len(files)
            n_dirs += len(dirs)
            for f_ in files:
                filepath = os.path.join(root, f_)
                if os.path.exists(filepath):
                    total_size += os.path.getsize(filepath)

        extra = (
            f"Bevat: {n_files} bestanden, {n_dirs} submappen\n"
            f"Inhoud: {human_size(total_size)}\n"
        )

    try:
        owner = pwd.getpwuid(st.st_uid).pw_name
        group = grp.getgrgid(st.st_gid).gr_name
    except (KeyError, OSError):
        owner, group = str(st.st_uid), str(st.st_gid)

    info = (
            f"Naam: {os.path.basename(path)}\n"
            f"Locatie: {os.path.dirname(path)}\n"
            f"Type: {_file_type_label(path, st)}\n"
            f"Grootte: {size_txt}\n"
            + extra +
            f"Gewijzigd: {datetime.fromtimestamp(st.st_mtime):%d-%m-%Y %H:%M:%S}\n"
            f"Rechten: {statmod.filemode(lst.st_mode)} "
            f"(octaal {oct(lst.st_mode & 0o777)})\n"
            f"Eigenaar: {owner} ({st.st_uid})    Groep: {group} ({st.st_gid})"
    )

    # Hier komt het dialoogvenster (dlg) dat je net al had...

    dlg = QDialog(parent)
    dlg.setWindowTitle(f"Eigenschappen — {os.path.basename(path)}")
    lay = QVBoxLayout(dlg)
    txt = QPlainTextEdit(info)
    txt.setReadOnly(True)
    txt.setStyleSheet("QPlainTextEdit { border: none; background: transparent; }")
    lay.addWidget(txt)
    btns = QDialogButtonBox(QDialogButtonBox.StandardButton.Close)
    btns.rejected.connect(dlg.reject)
    btns.clicked.connect(dlg.close)
    lay.addWidget(btns)
    dlg.exec()

def show_properties_dialog_multi(parent, paths):
    """Eigenschappen van een MEERDERE selectie: totalen optellen."""
    n_items = len(paths)
    n_files = n_dirs = errs = 0
    total = 0
    for p in paths:
        try:
            st = os.stat(p)
        except OSError:
            errs += 1
            continue
        if statmod.S_ISDIR(os.lstat(p).st_mode):
            n_dirs += 1
            for r, _d, fs in os.walk(p):
                n_files += len(fs)
                for f_ in fs:
                    fp = os.path.join(r, f_)
                    try:
                        total += os.path.getsize(fp)
                    except OSError:
                        pass
        else:
            n_files += 1
            total += st.st_size
    locatie = os.path.dirname(paths[0]) if paths else ""
    info = (
        f"Geselecteerd: {n_items} item(pen)\n"
        f"  waarvan {n_files} bestanden en {n_dirs} mappen\n"
        f"Locatie: {locatie}\n"
        f"Totale grootte: {human_size(total)}\n"
        + (f"⚠ {errs} item(s) niet leesbaar (overgeslagen)\n" if errs else "")
    )
    dlg = QDialog(parent)
    dlg.setWindowTitle(f"Eigenschappen — {n_items} item(s)")
    lay = QVBoxLayout(dlg)
    txt = QPlainTextEdit(info)
    txt.setReadOnly(True)
    txt.setStyleSheet("QPlainTextEdit { border: none; background: transparent; }")
    lay.addWidget(txt)
    btns = QDialogButtonBox(QDialogButtonBox.StandardButton.Close)
    btns.rejected.connect(dlg.reject)
    btns.clicked.connect(dlg.close)
    lay.addWidget(btns)
    dlg.exec()


def file_is_executable(path):
    try:
        return bool(os.stat(path).st_mode & 0o111)
    except OSError:
        return False


def set_executable(path, on):
    st = os.lstat(path)
    mode = st.st_mode | 0o111 if on else st.st_mode & ~0o111
    os.chmod(path, mode)


_http_server = {"server": None, "dir": None, "port": None}


def http_share_start(directory):
    """Start a tiny HTTP file server for directory. Returns URL or error."""
    import socket
    from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

    if _http_server["server"] is not None:
        return f"http://{_http_server.get('ip', 'localhost')}:" \
               f"{_http_server['port']}/"

    class Handler(SimpleHTTPRequestHandler):
        def __init__(self, *a, **kw):
            super().__init__(*a, directory=directory, **kw)

    port = 8000
    for attempt in range(20):
        try:
            srv = ThreadingHTTPServer(("", port), Handler)
            break
        except OSError:
            port += 1
    else:
        return "Geen vrije poort gevonden"

    ip = "127.0.0.1"
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
    except OSError:
        pass
    srv.daemon_threads = True
    _http_server.update({"server": srv, "dir": directory,
                         "port": port, "ip": ip})
    import threading

    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return f"http://{ip}:{port}/"


def http_share_stop():
    srv = _http_server["server"]
    if srv is not None:
        srv.shutdown()
        _http_server.update({"server": None, "dir": None, "port": None})


def samba_available():
    return shutil.which("net") is not None


D_HTTP = _http_server  # alias used by FileView share dialog


def samba_share_add(directory):
    name = os.path.basename(directory.rstrip("/")).replace(" ", "_")
    r = _run_cmd(["net", "usershare", "add", name, directory,
                  "Gedeeld via Lopus", "Everyone:r"])
    if r is None:
        return None, "net-commando mislukt"
    if r.returncode != 0:
        return None, r.stderr.strip() or r.stdout.strip() or "onbekende fout"
    return name, None


def samba_shares_for(directory):
    """Return share names that point at `directory` ([] if none/error)."""
    shares = samba_all_shares()
    target = directory.rstrip("/")
    return [name for name, spath in shares
            if os.path.abspath(spath) == target]


def samba_all_shares():
    """Return list of (share_name, path) for every Samba usershare."""
    r = _run_cmd(["net", "usershare", "info"])
    if r is None or r.returncode != 0:
        return []
    out = []
    current = None
    share_path = None
    for line in r.stdout.splitlines():
        line = line.strip()
        if line.startswith("[") and line.endswith("]"):
            if current and share_path:
                out.append((current, share_path))
            current = line[1:-1]
            share_path = None
        elif "=" in line:
            k, v = line.split("=", 1)
            if k.strip().lower() == "path":
                share_path = v.strip()
    if current and share_path:
        out.append((current, share_path))
    return out


def samba_share_delete(name):
    r = _run_cmd(["net", "usershare", "delete", name])
    return r is not None and r.returncode == 0




class ConflictDialog(QDialog):
    """Conflict-dialoog: Overschrijven / Hernoemen / Overslaan."""

    OVERWRITE, RENAME, SKIP = "overwrite", "rename", "skip"

    def __init__(self, src, dst, parent=None, apply_all=False):
        super().__init__(parent)
        self.setWindowTitle(tr("Bestand bestaat al"))
        self.action = self.SKIP
        self.apply_all = False
        lay = QVBoxLayout(self)

        def stat_line(p):
            try:
                st = os.stat(p)
                return (f"{human_size(st.st_size)}, gewijzigd "
                        f"{datetime.fromtimestamp(st.st_mtime):%d-%m-%Y %H:%M}")
            except OSError:
                return "onleesbaar"

        txt = QLabel(
            f"<b>{os.path.basename(dst)}</b> bestaat al in de doelmap.<br><br>"
            f"Bron: {os.path.basename(src)}<br>"
            f"<small>&nbsp;&nbsp;{stat_line(src)}</small><br>"
            f"Doel: {os.path.basename(dst)}<br>"
            f"<small>&nbsp;&nbsp;{stat_line(dst)}</small><br><br>"
            "Wat wil je doen?")
        lay.addWidget(txt)

        btns = QVBoxLayout()
        row_map = []
        for label, act in (
                ("Overschrijven", self.OVERWRITE),
                ("Hernoemen (automatisch nummeren)", self.RENAME),
                ("Overslaan", self.SKIP)):
            b = QPushButton(label)
            b.clicked.connect(lambda _c=False, a=act: self._pick(a))
            btns.addWidget(b)
            row_map.append(b)
        lay.addLayout(btns)

        self.chk_all = QCheckBox(tr("Toepassen op alle conflicten"))
        if apply_all:
            self.chk_all.setChecked(True)
        lay.addWidget(self.chk_all)

        cancel = QPushButton(tr("Annuleren (hele actie afbreken)"))
        cancel.clicked.connect(self.reject)
        lay.addWidget(cancel)

    def _pick(self, action):
        self.action = action
        self.apply_all = self.chk_all.isChecked()
        self.accept()


def resolve_conflicts(pairs, parent=None):
    """Filter (src, dst)-paren via de conflict-dialoog.

    Returnt (jobs, cancelled) waarbij jobs de definitieve (src, dst)-lijst
    is; botsingen worden volgens de gebruikerskeuze overschreven,
    automatisch hernoemd of overgeslagen."""
    jobs = []
    # Eerst alle bestaande doelen verzamelen
    collisions = [(s, d) for s, d in pairs if os.path.exists(d)]
    plain = [(s, d) for s, d in pairs if not os.path.exists(d)]
    all_action = None  # keuze bij "toepassen op alle conflicten"
    for src, dst in collisions:
        if all_action is not None:
            action = all_action
        else:
            dlg = ConflictDialog(src, dst, parent)
            if dlg.exec() != QDialog.DialogCode.Accepted:
                return [], True  # gebruiker brak hele actie af
            action = dlg.action
            if dlg.apply_all:
                all_action = action
        if action == ConflictDialog.OVERWRITE:
            jobs.append((src, dst))
        elif action == ConflictDialog.RENAME:
            jobs.append((src, unique_path(dst)))
        # SKIP: weglaten
    jobs.extend(plain)
    return jobs, False


# ---------- bestandsbeheer-gereedschap ----------

ARCHIVE_EXTS = {".zip", ".7z", ".tar", ".tgz", ".tbz2", ".txz"}


def is_archive(path):
    if not os.path.isfile(path):
        return False
    low = path.lower()
    if low.endswith((".tar.gz", ".tar.bz2", ".tar.xz")):
        return True
    return os.path.splitext(low)[1] in ARCHIVE_EXTS


def _7z_available():
    return shutil.which("7z") is not None or shutil.which("7za") is not None


def _7z_bin():
    return shutil.which("7z") or shutil.which("7za")


def archive_entries(path):
    """Lijst van (naam, grootte, is_map) in het archief."""
    low = path.lower()
    entries = []
    if low.endswith(".zip"):
        with zipfile.ZipFile(path) as z:
            for info in z.infolist():
                entries.append((info.filename, info.file_size,
                                info.is_dir()))
    elif low.endswith((".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2",
                       ".tar.xz", ".txz")):
        with tarfile.open(path, "r:*") as t:
            for m in t:
                entries.append((m.name, m.size, m.isdir()))
    elif low.endswith(".7z") and _7z_bin():
        r = subprocess.run([_7z_bin(), "l", "-slt", path],
                           capture_output=True, text=True, timeout=60)
        name = None
        size = 0
        isdir = False
        for line in r.stdout.splitlines():
            if line.startswith("Path = "):
                if name is not None:
                    entries.append((name, size, isdir))
                name, size, isdir = line[7:], 0, False
            elif name is not None:
                if line.startswith("Size = "):
                    try:
                        size = int(line[7:])
                    except ValueError:
                        size = 0
                elif line.startswith("Attributes ="):
                    isdir = "D" in line.split("=", 1)[1][:20]
        if name is not None:
            entries.append((name, size, isdir))
    return entries


def archive_extract(path, members, dest):
    """Pak (geselecteerde) leden uit naar dest."""
    low = path.lower()
    os.makedirs(dest, exist_ok=True)
    if low.endswith(".zip"):
        with zipfile.ZipFile(path) as z:
            for m in members:
                z.extract(m, dest)
    elif low.endswith((".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2",
                       ".tar.xz", ".txz")):
        with tarfile.open(path, "r:*") as t:
            t.extractall(dest, members=[m for m in t if m.name in members])
    elif low.endswith(".7z"):
        bin7 = _7z_bin()
        if bin7:
            subprocess.run([bin7, "x", "-y", f"-o{dest}", path, "--"]
                           + list(members), timeout=600, check=False)


ARCHIVE_FORMATS = [  # (extensie, menulabel)
    (".7z", "7z — kleinste, traagste"),
    (".zip", "zip — standaard, breed ondersteund"),
    (".tar.gz", "tar.gz — goed voor Linux-backups"),
    (".tar.bz2", "tar.bz2 — kleiner dan gz, trager"),
    (".tar.xz", "tar.xz — het kleinste van de tar-familie"),
]


def create_archive(src_paths, archive_path, level=5):
    """Maak een archief van de selectie (mappen recursief).

    level: 0 = opslaan (geen compressie), 1 = snel, 5 = normaal,
    9 = maximaal. Formaat volgt uit de extensie van archive_path."""
    low = archive_path.lower()
    if low.endswith(".7z"):
        bin7 = _7z_bin()
        if not bin7:
            raise RuntimeError(
                "7z is niet geïnstalleerd (sudo apt install p7zip-full)")
        r = subprocess.run(
            [bin7, "a", "-y", f"-mx={level}", archive_path, "--"]
            + list(src_paths),
            capture_output=True, text=True, timeout=3600)
        if r.returncode != 0:
            raise RuntimeError(r.stderr.strip() or "7z mislukte")
    elif low.endswith(".zip"):
        if level <= 0:
            comp, clevel = zipfile.ZIP_STORED, None
        else:
            comp, clevel = zipfile.ZIP_DEFLATED, min(level, 9)
        with zipfile.ZipFile(archive_path, "w", comp) as z:
            for src in src_paths:
                if os.path.isdir(src) and not os.path.islink(src):
                    for root, _dirs, files in os.walk(src):
                        for f in files:
                            full = os.path.join(root, f)
                            z.write(full, os.path.relpath(
                                full, os.path.dirname(src)),
                                compresslevel=clevel)
                else:
                    z.write(src, os.path.basename(src),
                            compresslevel=clevel)
    elif low.endswith((".tar.gz", ".tgz")):
        with tarfile.open(archive_path, "w:gz",
                          compresslevel=max(1, min(level, 9))) as t:
            for src in src_paths:
                t.add(src, arcname=os.path.basename(src.rstrip("/")))
    elif low.endswith((".tar.bz2", ".tbz2")):
        with tarfile.open(archive_path, "w:bz2",
                          compresslevel=max(1, min(level, 9))) as t:
            for src in src_paths:
                t.add(src, arcname=os.path.basename(src.rstrip("/")))
    elif low.endswith((".tar.xz", ".txz")):
        with tarfile.open(archive_path, "w:xz",
                          preset=max(1, min(level, 9))) as t:
            for src in src_paths:
                t.add(src, arcname=os.path.basename(src.rstrip("/")))
    else:
        raise ValueError(f"Onbekend archiefformaat: {archive_path}")


class ArchiveCreateDialog(QDialog):
    """Inpak-dialoog: naam, formaat en compressie-sterkte kiezen."""

    STRENGTHS = [  # (level, label)
        (0, "0 — Opslaan (geen compressie; snel, ideaal voor comics/jpg)"),
        (1, "1 — Snel (klein verschil met opslaan)"),
        (5, "5 — Normaal (aanbevolen)"),
        (9, "9 — Maximaal (kleinste, langzaamst)"),
    ]

    def __init__(self, base_name, parent=None, sevenz_ok=True):
        super().__init__(parent)
        self.setWindowTitle("Archief aanmaken")
        lay = QVBoxLayout(self)
        grid = QGridLayout()
        grid.addWidget(QLabel(tr("Naam:")), 0, 0)
        self.name_edit = QLineEdit(base_name + ".7z")
        grid.addWidget(self.name_edit, 0, 1)
        grid.addWidget(QLabel("Formaat:"), 1, 0)
        self.fmt_combo = QComboBox()
        for ext, label in ARCHIVE_FORMATS:
            self.fmt_combo.addItem(label, ext)
        if not sevenz_ok:
            self.fmt_combo.model().item(0).setEnabled(False)
        self.fmt_combo.setCurrentIndex(1 if not sevenz_ok else 0)
        grid.addWidget(self.fmt_combo, 1, 1)
        grid.addWidget(QLabel("Inpaksterkte:"), 2, 0)
        self.level_combo = QComboBox()
        for lvl, label in self.STRENGTHS:
            self.level_combo.addItem(label, lvl)
        self.level_combo.setCurrentIndex(2)  # normaal
        grid.addWidget(self.level_combo, 2, 1)
        lay.addLayout(grid)
        self.hint = QLabel("")
        self.hint.setWordWrap(True)
        lay.addWidget(self.hint)
        btns = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok
            | QDialogButtonBox.StandardButton.Cancel)
        btns.accepted.connect(self.accept)
        btns.rejected.connect(self.reject)
        lay.addWidget(btns)

        def sync_ext():
            ext = self.fmt_combo.currentData()
            base, _old = os.path.splitext(self.name_edit.text())
            if _old and self.name_edit.text().lower().endswith(
                    tuple(e for e, _ in ARCHIVE_FORMATS)):
                self.name_edit.setText(base + ext)
            lvl = self.level_combo.currentData()
            if lvl == 0:
                self.hint.setText(
                    "Tip: 'Opslaan' is perfect voor bestanden die al "
                    "ingepakt zijn (comics, jpg, video) — hij gaat dan "
                    "veel sneller en de bestanden worden niet kleiner.")
            else:
                self.hint.setText("")

        self.fmt_combo.currentIndexChanged.connect(sync_ext)
        self.level_combo.currentIndexChanged.connect(sync_ext)
        sync_ext()

    def result_name(self):
        return self.name_edit.text().strip()

    def result_level(self):
        return self.level_combo.currentData()


class ArchiveDialog(QDialog):
    """Archieven als map: inhoud bekijken en uitpakken."""

    def __init__(self, archive_path, parent=None):
        super().__init__(parent)
        self.archive = archive_path
        self.setWindowTitle(f"Archief — {os.path.basename(archive_path)}")
        self.resize(560, 440)
        lay = QVBoxLayout(self)
        self.tree = QTreeWidget()
        self.tree.setHeaderLabels(["Naam", "Grootte", "Type"])
        self.tree.setColumnWidth(0, 340)
        self.tree.setRootIsDecorated(False)
        self.tree.setAlternatingRowColors(True)
        lay.addWidget(self.tree, 1)
        row = QHBoxLayout()
        btn_sel = QPushButton("Uitpakken (selectie)...")
        btn_all = QPushButton(tr("Alles uitpakken..."))
        btn_close = QPushButton(tr("Sluiten"))
        btn_sel.clicked.connect(self._extract_sel)
        btn_all.clicked.connect(self._extract_all)
        btn_close.clicked.connect(self.accept)
        row.addWidget(btn_sel)
        row.addWidget(btn_all)
        row.addStretch(1)
        row.addWidget(btn_close)
        lay.addLayout(row)
        self.entries = []
        try:
            self.entries = archive_entries(archive_path)
        except Exception as e:  # noqa: BLE001
            QMessageBox.warning(self, APP_NAME, f"Kan archief niet lezen:\n{e}")
        for name, size, isdir in self.entries:
            it = QTreeWidgetItem([
                name, "-" if isdir else human_size(size),
                "Map" if isdir else "Bestand"])
            it.setData(0, Qt.ItemDataRole.UserRole, name)
            self.tree.addTopLevelItem(it)
        self.tree.sortItems(0, Qt.SortOrder.AscendingOrder)

    def _choose_dest(self):
        dlg = DirPickerDialog(os.path.expanduser("~"), self)
        if dlg.exec() and dlg.chosen:
            return dlg.chosen
        return None

    def _extract_sel(self):
        names = [i.data(0, Qt.ItemDataRole.UserRole)
                 for i in self.tree.selectedItems()]
        if not names:
            return
        dest = self._choose_dest()
        if dest:
            try:
                archive_extract(self.archive, names, dest)
                QMessageBox.information(self, APP_NAME,
                                        f"Uitgepakt naar:\n{dest}")
            except Exception as e:  # noqa: BLE001
                QMessageBox.warning(self, APP_NAME, str(e))

    def _extract_all(self):
        dest = self._choose_dest()
        if dest:
            try:
                archive_extract(
                    self.archive,
                    [e[0] for e in self.entries if not e[2]], dest)
                QMessageBox.information(self, APP_NAME,
                                        f"Uitgepakt naar:\n{dest}")
            except Exception as e:  # noqa: BLE001
                QMessageBox.warning(self, APP_NAME, str(e))


def split_file(path, part_mb):
    """Splits bestand in delen van part_mb MB; geeft deelnamen terug."""
    part = part_mb * 1024 * 1024
    parts = []
    with open(path, "rb") as f:
        i = 1
        while True:
            chunk = f.read(part)
            if not chunk:
                break
            pname = f"{path}.{i:03d}"
            with open(pname, "wb") as out:
                out.write(chunk)
            parts.append(pname)
            i += 1
    return parts


def merge_parts(first_part):
    """Voeg .001-delen samen tot het originele bestand."""
    base = first_part[: first_part.rfind(".")]
    if not base:
        raise ValueError(f"Geen geldig deel: {first_part}")
    with open(base, "wb") as out:
        i = 1
        while True:
            pname = f"{base}.{i:03d}"
            if not os.path.exists(pname):
                break
            with open(pname, "rb") as f:
                shutil.copyfileobj(f, out, 4 * 1024 * 1024)
            i += 1
    return base


class DuplicateFinderDialog(QDialog):
    """Zoek dubbele bestanden (grootte -> hash) in een mapboom."""

    LIMIT_FILES = 20000
    LIMIT_SECS = 15.0

    def __init__(self, directory, parent=None, trash_fn=None):
        super().__init__(parent)
        self.directory = directory
        self.trash_fn = trash_fn or (lambda ps: [])
        self.setWindowTitle(f"Dubbele bestanden — {directory}")
        self.resize(640, 480)
        lay = QVBoxLayout(self)
        self.tree = QTreeWidget()
        self.tree.setHeaderLabels(["Bestand", "Grootte"])
        self.tree.setColumnWidth(0, 440)
        lay.addWidget(self.tree, 1)
        self.status = QLabel(tr("Zoeken..."))
        lay.addWidget(self.status)
        row = QHBoxLayout()
        btn_sel = QPushButton(tr("Alles behalve eerste selecteren"))
        btn_del = QPushButton(tr("Verwijderen (prullenbak)"))
        btn_close = QPushButton(tr("Sluiten"))
        btn_sel.clicked.connect(self._select_dupes)
        btn_del.clicked.connect(self._delete)
        btn_close.clicked.connect(self.accept)
        row.addWidget(btn_sel)
        row.addWidget(btn_del)
        row.addStretch(1)
        row.addWidget(btn_close)
        lay.addLayout(row)
        self.groups = []  # list of list-of-paths
        self._scan()

    def _hash(self, path):
        h = hashlib.md5()
        try:
            with open(path, "rb") as f:
                while True:
                    chunk = f.read(1024 * 1024)
                    if not chunk:
                        break
                    h.update(chunk)
        except OSError:
            return None
        return h.hexdigest()

    def _scan(self):
        start = time.monotonic()
        by_size = {}
        count = 0
        for root, dirs, files in os.walk(self.directory):
            dirs[:] = [d for d in dirs if not d.startswith(".")]
            for f in files:
                if count >= self.LIMIT_FILES:
                    break
                if f.startswith("."):
                    continue
                p = os.path.join(root, f)
                try:
                    sz = os.path.getsize(p)
                except OSError:
                    continue
                if sz == 0:
                    continue
                by_size.setdefault(sz, []).append(p)
                count += 1
            if count >= self.LIMIT_FILES:
                break
        by_hash = {}
        for ps in [ps for ps in by_size.values() if len(ps) > 1]:
            for p in ps:
                if time.monotonic() - start > self.LIMIT_SECS:
                    break
                h = self._hash(p)
                if h:
                    by_hash.setdefault((os.path.getsize(p), h), []).append(p)
        for _k, ps in by_hash.items():
            if len(ps) > 1:
                self.groups.append(ps)
                for p in ps:
                    it = QTreeWidgetItem(
                        [p, human_size(os.path.getsize(p))])
                    it.setData(0, Qt.ItemDataRole.UserRole, p)
                    self.tree.addTopLevelItem(it)
        n = sum(len(g) - 1 for g in self.groups)
        self.status.setText(
            f"{len(self.groups)} groep(en), {n} overbodige bestanden"
            if self.groups else "Geen duplicaten gevonden.")

    def _select_dupes(self):
        """Selecteer in elke groep (zelfde grootte) alles behalve de eerste."""
        prev_size = None
        for i in range(self.tree.topLevelItemCount()):
            it = self.tree.topLevelItem(i)
            p = it.data(0, Qt.ItemDataRole.UserRole)
            try:
                size = os.path.getsize(p)
            except OSError:
                size = -1
            it.setSelected(prev_size == size)
            prev_size = size

    def _delete(self):
        paths = [i.data(0, Qt.ItemDataRole.UserRole)
                 for i in self.tree.selectedItems()]
        if not paths:
            return
        r = QMessageBox.question(
            self, APP_NAME,
            f"{len(paths)} bestand(en) naar de prullenbak?")
        if r != QMessageBox.StandardButton.Yes:
            return
        failed = self.trash_fn(paths)
        if failed:
            QMessageBox.warning(self, APP_NAME, "Niet gelukt:\n"
                                + "\n".join(str(f) for f in failed))
        self.tree.clear()
        self.groups = []
        self._scan()


class SyncDialog(QDialog):
    """Vergelijk twee mappen en synchroniseer links↔rechts.

    De vergelijking (recursief os.walk over beide bomen) draait op de
    achtergrond: grote mappen blokkeren de interface niet."""

    def __init__(self, left_dir, right_dir, parent=None, run_transfer=None):
        super().__init__(parent)
        self.left_dir = left_dir
        self.right_dir = right_dir
        self.run_transfer = run_transfer  # fn(jobs, move=False)
        self.setWindowTitle(tr("Mappen vergelijken"))
        self.resize(680, 500)
        lay = QVBoxLayout(self)
        lay.addWidget(QLabel(f"Links:  {left_dir}\nRechts: {right_dir}"))
        self.tree = QTreeWidget()
        self.tree.setHeaderLabels(["Bestand", "Status", "Links", "Rechts"])
        self.tree.setColumnWidth(0, 320)
        lay.addWidget(self.tree, 1)
        self.status = QLabel(
            tr("⏳ Bezig met vergelijken (kan even duren bij grote mappen)..."))
        lay.addWidget(self.status)
        row = QHBoxLayout()
        self.btn_lr = QPushButton(tr("Links → Rechts"))
        self.btn_rl = QPushButton(tr("Rechts → Links"))
        btn_close = QPushButton(tr("Sluiten"))
        self.btn_lr.clicked.connect(lambda: self._sync(True))
        self.btn_rl.clicked.connect(lambda: self._sync(False))
        btn_close.clicked.connect(self.accept)
        row.addWidget(self.btn_lr)
        row.addWidget(self.btn_rl)
        row.addStretch(1)
        row.addWidget(btn_close)
        lay.addLayout(row)
        # Knoppen pas actief als de vergelijking klaar is
        self.btn_lr.setEnabled(False)
        self.btn_rl.setEnabled(False)
        self.to_lr, self.to_rl = [], []
        self._cancel_flag = False
        btn_cancel = QPushButton(tr("Annuleren"))
        btn_cancel.clicked.connect(self._cancel_compare)
        row.addWidget(btn_cancel)
        # Recursieve vergelijking op de achtergrond
        self._start_compare()

    def _cancel_compare(self):
        self._cancel_flag = True
        self.status.setText(tr("⏳ Bezig met afbreken..."))

    def _start_compare(self):
        self._cancel_flag = False
        self.status.setText(
            tr("⏳ Bezig met vergelijken (kan even duren bij grote mappen)..."))
        self.tree.clear()
        self.btn_lr.setEnabled(False)
        self.btn_rl.setEnabled(False)
        self._thread = FuncThread(
            lambda: (self._collect(self.left_dir),
                     self._collect(self.right_dir)))
        self._thread.done.connect(self._on_compared)
        self._thread.finished.connect(self._thread.deleteLater)
        self._thread.start()

    def _collect(self, base):
        out = {}
        count = 0
        for root, dirs, files in os.walk(base):
            if self._cancel_flag:
                break
            dirs[:] = [d for d in dirs if not d.startswith(".")]
            for f in files:
                if self._cancel_flag:
                    break
                if f.startswith(".") or count > 20000:
                    continue
                full = os.path.join(root, f)
                rel = os.path.relpath(full, base)
                try:
                    st = os.stat(full)
                except OSError:
                    continue
                out[rel] = (st.st_size, st.st_mtime)
                count += 1
        return out

    def _on_compared(self, result):
        if self._cancel_flag:
            self.status.setText("Vergelijken afgebroken.")
            return
        if isinstance(result, Exception):
            self.status.setText(f"⚠ Fout bij vergelijken: {result}")
            return
        L, R = result
        self.tree.clear()
        self.to_lr, self.to_rl = [], []
        for rel in sorted(set(L) | set(R), key=str.lower):
            if rel in L and rel not in R:
                status, lr, rr = ("alleen links",
                                  human_size(L[rel][0]), "-")
                self.to_lr.append(rel)
            elif rel in R and rel not in L:
                status, lr, rr = ("alleen rechts", "-",
                                  human_size(R[rel][0]))
                self.to_rl.append(rel)
            else:
                ls, lm = L[rel]
                rs, rm = R[rel]
                lr, rr = human_size(ls), human_size(rs)
                if lm > rm + 1:
                    status = "nieuwer links"
                    self.to_lr.append(rel)
                elif rm > lm + 1:
                    status = "nieuwer rechts"
                    self.to_rl.append(rel)
                elif ls != rs:
                    status = "verschillend"
                else:
                    continue  # gelijke bestanden niet tonen
            self.tree.addTopLevelItem(
                QTreeWidgetItem([rel, status, lr, rr]))
        n = self.tree.topLevelItemCount()
        self.status.setText(
            f"{len(self.to_lr)} bestanden naar rechts te kopiëren, "
            f"{len(self.to_rl)} naar links  |  {n} verschil(len) gevonden")
        self.btn_lr.setEnabled(bool(self.to_lr))
        self.btn_rl.setEnabled(bool(self.to_rl))

    def _sync(self, left_to_right):
        if not self.run_transfer:
            return
        src_base = self.left_dir if left_to_right else self.right_dir
        dst_base = self.right_dir if left_to_right else self.left_dir
        rels = self.to_lr if left_to_right else self.to_rl
        jobs = [(os.path.join(src_base, r), os.path.join(dst_base, r))
                for r in rels]
        jobs = [(s, d) for s, d in jobs if os.path.isfile(s)]
        if jobs:
            self.run_transfer(jobs, False)
            self._start_compare()


class SearchDialog(QDialog):
    """Geavanceerd zoeken met filters (naam, tekst, grootte, datum)."""

    LIMIT = 3000
    LIMIT_SECS = 15.0

    def __init__(self, directory, parent=None, open_dir_cb=None):
        super().__init__(parent)
        import fnmatch
        self._fnmatch = fnmatch.fnmatch
        self.directory = directory
        self.open_dir_cb = open_dir_cb
        self.setWindowTitle(f"Zoeken in {directory}")
        self.resize(640, 500)
        lay = QVBoxLayout(self)
        grid = QGridLayout()
        grid.addWidget(QLabel(tr("Naam bevat (of *.patroon):")), 0, 0)
        self.name_edit = QLineEdit()
        grid.addWidget(self.name_edit, 0, 1)
        grid.addWidget(QLabel(tr("Inhoud bevat:")), 1, 0)
        self.text_edit = QLineEdit()
        grid.addWidget(self.text_edit, 1, 1)
        grid.addWidget(QLabel(tr("Min. grootte (KB):")), 2, 0)
        self.min_spin = QSpinBox()
        self.min_spin.setRange(0, 10_000_000)
        grid.addWidget(self.min_spin, 2, 1)
        grid.addWidget(QLabel(tr("Max. grootte (KB, 0=geen):")), 3, 0)
        self.max_spin = QSpinBox()
        self.max_spin.setRange(0, 10_000_000)
        grid.addWidget(self.max_spin, 3, 1)
        grid.addWidget(QLabel(tr("Gewijzigd binnen (dagen, 0=altijd):")), 4, 0)
        self.days_spin = QSpinBox()
        self.days_spin.setRange(0, 100000)
        grid.addWidget(self.days_spin, 4, 1)
        lay.addLayout(grid)
        btn_search = QPushButton(tr("Zoeken"))
        btn_search.clicked.connect(self._search)
        lay.addWidget(btn_search)
        self.tree = QTreeWidget()
        self.tree.setHeaderLabels([tr("Bestand"), tr("Grootte"), tr("Gewijzigd")])
        self.tree.setColumnWidth(0, 400)
        self.tree.itemDoubleClicked.connect(self._open_item)
        lay.addWidget(self.tree, 1)
        self.status = QLabel("")
        lay.addWidget(self.status)

    def _match_name(self, name):
        pat = self.name_edit.text().strip().lower()
        if not pat:
            return True
        if "*" in pat or "?" in pat:
            return self._fnmatch(name.lower(), pat)
        return pat in name.lower()

    def _search(self):
        self.tree.clear()
        start = time.monotonic()
        needle = self.text_edit.text().strip().lower()
        minb = self.min_spin.value() * 1024
        maxb = self.max_spin.value() * 1024
        days = self.days_spin.value()
        n = 0
        for root, dirs, files in os.walk(self.directory):
            dirs[:] = [d for d in dirs if not d.startswith(".")]
            for f in files:
                if (n >= self.LIMIT
                        or time.monotonic() - start > self.LIMIT_SECS):
                    self.status.setText(
                        "Limiet bereikt — resultaten incompleet.")
                    return
                if f.startswith(".") or not self._match_name(f):
                    continue
                full = os.path.join(root, f)
                try:
                    st = os.stat(full)
                except OSError:
                    continue
                if minb and st.st_size < minb:
                    continue
                if maxb and st.st_size > maxb:
                    continue
                if days and (time.time() - st.st_mtime) > days * 86400:
                    continue
                if needle:
                    try:
                        with open(full, "rb") as fh:
                            data = fh.read(2 * 1024 * 1024)
                        if needle.encode(errors="ignore") not in data.lower():
                            continue
                    except (OSError, MemoryError):
                        continue
                self.tree.addTopLevelItem(QTreeWidgetItem([
                    full, human_size(st.st_size),
                    datetime.fromtimestamp(st.st_mtime)
                    .strftime("%d-%m-%Y %H:%M")]))
                n += 1
        self.status.setText(f"{n} resultaat(en)")

    def _open_item(self, item, _col):
        if self.open_dir_cb:
            self.open_dir_cb(os.path.dirname(item.text(0)))
            self.accept()


class CopyWorker(QThread):
    progress = pyqtSignal(int, str)   # percentage, current filename
    info = pyqtSignal(str)            # human readable speed/ETA line
    finished_ok = pyqtSignal(bool, str)

    CANCELLED_MSG = "__afgebroken__"

    def __init__(self, jobs, move=False):
        super().__init__()
        self.jobs = jobs  # list of (src, dst)
        self.move = move
        self._cancel_requested = False
        self._paused = False
        self._last_flush = 0
        # Naar een netwerk-mount (rclone/OneDrive e.d.) schrijven? Dan moet
        # de laatste buffer bij het sluiten nog geüpload worden.
        try:
            self.dst_remote = any(is_remote_path(d) for _s, d in jobs)
        except Exception:  # noqa: BLE001
            self.dst_remote = False

    def cancel(self):
        self._cancel_requested = True

    def pause(self):
        self._paused = True

    def resume(self):
        self._paused = False

    def _wait_if_paused(self):
        while self._paused and not self._cancel_requested:
            time.sleep(0.1)

    @staticmethod
    def _tree_size(path):
        try:
            if not os.path.isdir(path):
                return os.path.getsize(path)
            total = 0
            for root, _d, files in os.walk(path):
                for f in files:
                    try:
                        total += os.path.getsize(os.path.join(root, f))
                    except OSError:
                        continue
            return total
        except OSError:
            return 0

    @staticmethod
    def _fmt_eta(secs):
        secs = int(max(secs, 0))
        m, s = divmod(secs, 60)
        h, m = divmod(m, 60)
        return f"{h}:{m:02d}:{s:02d}" if h else f"{m}:{s:02d}"

    def _copy_file_chunked(self, src, dst, report=None):
        """Copy a file in 4MB chunks so cancel works mid-file.
        Calls report() after every chunk for a live progress bar.
        Returns False if cancelled (partial dst removed)."""
        os.makedirs(os.path.dirname(dst) or ".", exist_ok=True)
        try:
            with open(src, "rb") as fsrc, open(dst, "wb") as fdst:
                while True:
                    if self._cancel_requested:
                        try:
                            fdst.close()
                            os.remove(dst)
                        except OSError:
                            pass
                        return False
                    self._wait_if_paused()
                    chunk = fsrc.read(4 * 1024 * 1024)
                    if not chunk:
                        if self.dst_remote:
                            self.info.emit(
                                "⏳ Afronden: resterende data uploaden "
                                "(netwerk)...")
                        try:
                            fdst.flush()
                        except OSError:
                            pass
                        return True
                    fdst.write(chunk)
                    self._chunk_copied += len(chunk)
                    # Periodiek doorspoelen: bij rclone/OneDrive belandt de
                    # data anders onzichtbaar in de VFS-buffer (de balk lijkt
                    # vast te staan) en blokkeert een latere write volledig.
                    # Na een flush pakt de balk weer op en werkt Annuleren.
                    if (self._chunk_copied - self._last_flush
                            >= 16 * 1024 * 1024):
                        try:
                            fdst.flush()
                        except OSError:
                            pass
                        self._last_flush = self._chunk_copied
                    if report:
                        report()
        except Exception as e:  # noqa: BLE001
            raise e

    def run(self):
        self.info.emit("Grootte bepalen...")
        total = sum(self._tree_size(s) for s, _d in self.jobs)
        n_jobs = len(self.jobs)
        self._chunk_copied = 0
        start = time.monotonic()
        errors = []
        cancelled = False
        for i, (src, dst) in enumerate(self.jobs):
            self._wait_if_paused()
            if self._cancel_requested:
                cancelled = True
                break
            name = os.path.basename(src.rstrip("/"))
            sz = max(self._tree_size(src), 1)
            base_done = self._chunk_copied

            def report():
                now = time.monotonic()
                if now - last_ui_report[0] >= 0.15:
                    last_ui_report[0] = now
                    elapsed = max(now - start, 0.001)
                    speed = self._chunk_copied / elapsed
                    pct = min(int(self._chunk_copied * 100 / total), 100) \
                        if total else int((i + 1) * 100 / n_jobs)
                    eta = (total - self._chunk_copied) / speed if speed > 0 else 0
                    self.progress.emit(pct, name)
                    self.info.emit(
                        f"{human_size(self._chunk_copied)} van "
                        f"{human_size(total)}  •  {human_size(speed)}/s  •  "
                        f"resterend ~{self._fmt_eta(eta)}"
                    )

            last_ui_report = [0.0]
            try:
                # Overschrijven: bij verplaatsen moet het bestaande doel
                # eerst weg (shutil.move weigert bestaande mappen).
                if self.move and os.path.exists(dst):
                    if os.path.isdir(dst) and not os.path.islink(dst):
                        shutil.rmtree(dst)
                    else:
                        os.remove(dst)
                if os.path.isdir(src) and not os.path.islink(src):
                    if self.move:
                        shutil.move(src, dst)
                    else:
                        # copy_function zorgt dat mappen óók per chunk
                        # rapporteren (live balk bij grote mappen)
                        shutil.copytree(
                            src, dst, dirs_exist_ok=True,
                            copy_function=lambda s, d:
                                self._copy_file_chunked(s, d, report) and d)
                elif self.move:
                    # Same filesystem: instant rename
                    shutil.move(src, dst)
                else:
                    if not self._copy_file_chunked(src, dst, report):
                        cancelled = True
                        break
                self._chunk_copied = base_done + sz
            except Exception as e:  # noqa: BLE001
                errors.append(f"{name}: {e}")
                self._chunk_copied = base_done + sz
            report()
        if cancelled:
            self.finished_ok.emit(True, self.CANCELLED_MSG)
        else:
            self.finished_ok.emit(not errors, "\n".join(errors))


# ---------- audio tags ----------
AUDIO_EXTS = {
    ".mp3", ".flac", ".ogg", ".oga", ".opus", ".m4a", ".m4b",
    ".aac", ".wav", ".wma", ".wv", ".ape",
}
_audio_cache = {}  # (path, mtime) -> info dict

AUDIO_COLUMN_DEFS = {  # key -> column label
    "title": "Titel",
    "artist": "Artiest",
    "album": "Album",
    "albumartist": "Album-artiest",
    "genre": "Genre",
    "date": "Jaar",
    "track": "Nr",
    "duration": "Duur",
    "bitrate": "Bitrate",
}


def get_audio_info(path):
    """Read audio tags/bitrate/duration via ffprobe. Cached per file+mtime."""
    try:
        mtime = os.stat(path).st_mtime
    except OSError:
        return None
    key = (path, mtime)
    if key in _audio_cache:
        return _audio_cache[key]
    info = {}
    r = _run_cmd(
        ["ffprobe", "-v", "quiet", "-print_format", "json", "-show_format", path],
        timeout=15,
    )
    if r is not None and r.returncode == 0 and r.stdout.strip():
        try:
            fmt = json.loads(r.stdout).get("format", {})
            tags = {k.lower(): v for k, v in (fmt.get("tags") or {}).items()}
            info["title"] = tags.get("title", "")
            info["artist"] = tags.get("artist", "")
            info["album"] = tags.get("album", "")
            info["albumartist"] = tags.get("album_artist", tags.get("albumartist", ""))
            info["genre"] = tags.get("genre", "")
            info["date"] = tags.get("date", tags.get("year", ""))
            info["track"] = tags.get("track", "")
            dur = fmt.get("duration")
            if dur:
                secs = float(dur)
                info["duration"] = f"{int(secs // 60)}:{int(secs % 60):02d}"
            br = fmt.get("bit_rate")
            if br:
                info["bitrate"] = f"{int(br) // 1000} kb/s"
        except (ValueError, TypeError, KeyError):
            pass
    _audio_cache[key] = info
    # keep cache bounded
    if len(_audio_cache) > 4000:
        _audio_cache.clear()
    return info


VIDEO_EXTS = {".mp4", ".mkv", ".avi", ".mov", ".webm", ".mpg", ".mpeg",
              ".wmv", ".flv", ".m4v", ".ts"}
IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".gif", ".bmp", ".webp", ".tif",
              ".tiff", ".heic", ".avif"}
_media_cache = {}


def get_media_info(path):
    """Rich media/file info via ffprobe (audio, video, images). Cached."""
    try:
        mtime = os.stat(path).st_mtime
    except OSError:
        return {}
    key = (path, mtime)
    if key in _media_cache:
        return _media_cache[key]
    info = {}
    r = _run_cmd(
        ["ffprobe", "-v", "quiet", "-print_format", "json",
         "-show_format", "-show_streams", path],
        timeout=15,
    )
    if r is not None and r.returncode == 0 and r.stdout.strip():
        try:
            data = json.loads(r.stdout)
            fmt = data.get("format", {})
            streams = data.get("streams", [])
            tags = {k.lower(): v for k, v in (fmt.get("tags") or {}).items()}
            for k, v in tags.items():
                info.setdefault(k, v)
            dur = fmt.get("duration")
            if dur:
                secs = float(dur)
                info["duration"] = f"{int(secs // 60)}:{int(secs % 60):02d}"
            br = fmt.get("bit_rate")
            if br:
                info["bitrate"] = f"{int(br) // 1000} kb/s"
            vst = next((s for s in streams
                        if s.get("codec_type") == "video"), None)
            ast = next((s for s in streams
                        if s.get("codec_type") == "audio"), None)
            if vst:
                w, h = vst.get("width"), vst.get("height")
                if w and h:
                    info["dimensions"] = f"{w}x{h}"
                if vst.get("codec_name"):
                    info["vcodec"] = vst["codec_name"]
                fps = vst.get("r_frame_rate")
                if fps and "/" in fps:
                    num, den = fps.split("/")
                    try:
                        info["fps"] = f"{round(int(num) / int(den))} fps"
                    except (ZeroDivisionError, ValueError):
                        pass
                if vst.get("display_aspect_ratio"):
                    info["aspect"] = vst["display_aspect_ratio"]
            if ast:
                if ast.get("codec_name"):
                    info["acodec"] = ast["codec_name"]
                if ast.get("sample_rate"):
                    info["sample_rate"] = f"{ast['sample_rate']} Hz"
                if ast.get("channels"):
                    info["channels"] = f"{ast['channels']} ch"
            if fmt.get("format_name"):
                info["format"] = fmt["format_name"]
            if tags.get("model"):
                info["model"] = tags["model"]
        except (ValueError, TypeError, KeyError):
            pass
    _media_cache[key] = info
    if len(_media_cache) > 4000:
        _media_cache.clear()
    return info


# Column library: (key, label, groups it belongs to)
COLUMN_LIBRARY = [
    # --- Audio ---
    ("title", "Titel", ("audio", "video")),
    ("artist", "Artiest", ("audio", "video")),
    ("album", "Album", ("audio",)),
    ("albumartist", "Album-artiest", ("audio",)),
    ("genre", "Genre", ("audio",)),
    ("date", "Jaar/Datum", ("audio", "video", "foto")),
    ("track", "Nr", ("audio",)),
    ("disc", "Schijfnummer", ("audio",)),
    ("composer", "Componist", ("audio",)),
    ("comment", "Opmerking", ("audio", "video")),
    ("copyright", "Copyright", ("audio", "video", "foto")),
    ("encoder", "Encoder", ("audio", "video")),
    ("language", "Taal", ("audio", "video")),
    ("duration", "Duur", ("audio", "video")),
    ("bitrate", "Bitrate", ("audio", "video")),
    ("acodec", "Audiocodec", ("audio",)),
    ("sample_rate", "Samplerate", ("audio",)),
    ("channels", "Kanalen", ("audio",)),
    # --- Video ---
    ("vcodec", "Videocodec", ("video",)),
    ("dimensions", "Afmetingen", ("video", "foto")),
    ("fps", "FPS", ("video",)),
    ("aspect", "Beeldverhouding", ("video",)),
    ("format", "Container", ("audio", "video")),
    # --- Foto ---
    ("model", "Camera", ("foto",)),
    # --- Overige ---
    ("mime", "MIME-type", ("overig",)),
    ("owner", "Eigenaar", ("overig",)),
    ("perms", "Rechten", ("overig",)),
]
COLUMN_LABELS = {k: lbl for k, lbl, _g in COLUMN_LIBRARY}
DEFAULT_TYPE_COLUMNS = {
    "audio": ["title", "artist", "album", "albumartist", "genre",
              "date", "track", "duration", "bitrate"],
    "video": ["duration", "bitrate", "dimensions", "vcodec", "fps"],
    "foto": ["dimensions", "date", "model"],
    "overig": ["mime", "owner", "perms"],
}
TYPE_LABELS = [("audio", "Audio"),
               ("video", "Video"),
               ("foto", "Foto's"),
               ("overig", "Diverse")]

SPECIAL_COLUMN_GETTERS = {
    "mime": get_mime_type,
    "owner": lambda p: _owner_name(os.lstat(p)),
    "perms": lambda p: statmod.filemode(os.lstat(p).st_mode)[1:10],
}


def _owner_name(st):
    import pwd

    try:
        return pwd.getpwuid(st.st_uid).pw_name
    except KeyError:
        return str(st.st_uid)


def get_column_value(key, path, is_dir):
    """Value for a library column; media info via ffprobe when applicable."""
    if is_dir:
        return ""
    if key in SPECIAL_COLUMN_GETTERS:
        try:
            return SPECIAL_COLUMN_GETTERS[key](path)
        except Exception:  # noqa: BLE001
            return ""
    ext = os.path.splitext(path)[1].lower()
    if ext in AUDIO_EXTS or ext in VIDEO_EXTS or ext in IMAGE_EXTS:
        return get_media_info(path).get(key, "")
    return ""


class SortableItem(QTreeWidgetItem):
    """Tree item that keeps folders on top regardless of sort direction,
    and sorts sizes numerically."""

    def _is_dir(self):
        e = self.data(COL_NAME, Qt.ItemDataRole.UserRole)
        return bool(e and e.get("isdir"))

    def __lt__(self, other):
        tree = self.treeWidget()
        col = tree.sortColumn() if tree else COL_NAME
        asc = (tree.header().sortIndicatorOrder() == Qt.SortOrder.AscendingOrder
               ) if tree else True
        s_dir, o_dir = self._is_dir(), other._is_dir()
        # Folders always above files, in BOTH sort directions
        if s_dir != o_dir:
            return s_dir if asc else o_dir
        # Numeric sort for the size column
        if col == COL_SIZE:
            try:
                s_val = float(self.data(COL_SIZE, Qt.ItemDataRole.UserRole + 1) or 0)
                o_val = float(other.data(COL_SIZE, Qt.ItemDataRole.UserRole + 1) or 0)
                if s_val != o_val:
                    return s_val < o_val
                # Tie-break on name
                return self.text(COL_NAME).lower().lstrip('.') < other.text(COL_NAME).lower().lstrip('.')
            except (TypeError, ValueError):
                pass
        # Chronological sort for the modified-date column
        if col == COL_MODIFIED:
            try:
                s_val = float(self.data(COL_MODIFIED, Qt.ItemDataRole.UserRole + 1)
                              or 0)
                o_val = float(other.data(COL_MODIFIED, Qt.ItemDataRole.UserRole + 1)
                              or 0)
                if s_val != o_val:
                    return s_val < o_val
                return self.text(COL_NAME).lower() < other.text(COL_NAME).lower()
            except (TypeError, ValueError):
                pass
        # Case-insensitive text comparison (negeer de punt voor sortering)
        return self.text(col).lower().lstrip('.') < other.text(col).lower().lstrip('.')


class PathBar(QWidget):
    """Breadcrumb path bar: every segment is clickable to navigate."""

    navigate = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        outer = QHBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(0)

        self.stack = QStackedWidget()
        # Page 0: breadcrumb buttons
        self.crumb = QWidget()
        self.crumb_layout = QHBoxLayout(self.crumb)
        self.crumb_layout.setContentsMargins(2, 0, 2, 0)
        self.crumb_layout.setSpacing(1)
        self.crumb_layout.addStretch(1)
        self.stack.addWidget(self.crumb)
        # Page 1: editable path
        self.edit = QLineEdit()
        self.edit.returnPressed.connect(self._apply_edit)
        self.edit.installEventFilter(self)
        self.stack.addWidget(self.edit)
        outer.addWidget(self.stack, 1)

        self.btn_edit = QPushButton("✎")
        self.btn_edit.setObjectName("navBtn")
        self.btn_edit.setFixedWidth(26)
        self.btn_edit.setToolTip(tr("Pad bewerken"))
        self.btn_edit.clicked.connect(self._start_edit)
        outer.addWidget(self.btn_edit)

    def eventFilter(self, obj, event):
        from PyQt6.QtCore import QEvent

        if obj is self.edit and event.type() == QEvent.Type.KeyPress:
            if event.key() == Qt.Key.Key_Escape:
                self.stack.setCurrentIndex(0)
                return True
        return super().eventFilter(obj, event)

    def set_path(self, path):
        self.edit.setText(path)
        # Rebuild breadcrumb
        while self.crumb_layout.count() > 1:
            item = self.crumb_layout.takeAt(0)
            w = item.widget()
            if w:
                w.deleteLater()
        norm = path.rstrip("/")
        parts = [p for p in norm.split("/") if p]
        acc = ""
        entries = [("🖥 /", "/")]
        for p in parts:
            acc += "/" + p
            entries.append((p, acc))
        for label, target in entries:
            btn = QPushButton(label)
            btn.setObjectName("crumbBtn")
            btn.setFlat(True)
            btn.setCursor(Qt.CursorShape.PointingHandCursor)
            btn.setStyleSheet(
                "QPushButton { border: none; padding: 0px 3px; }"
                "QPushButton:hover { text-decoration: underline; }"
            )
            # Never allow the layout to squeeze the label into truncation
            btn.setMinimumWidth(btn.sizeHint().width())
            btn.setSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Preferred)
            btn.clicked.connect(
                lambda _c=False, t=target: self.navigate.emit(t)
            )
            self.crumb_layout.insertWidget(self.crumb_layout.count() - 1, btn)
            if target != "/":
                sep = QLabel("/")
                self.crumb_layout.insertWidget(
                    self.crumb_layout.count() - 1, sep
                )

    def _start_edit(self):
        self.stack.setCurrentIndex(1)
        self.edit.setFocus()
        self.edit.selectAll()

    def _apply_edit(self):
        self.stack.setCurrentIndex(0)
        self.navigate.emit(os.path.expanduser(self.edit.text().strip()))


class _ReverseStr:
    """Wrapper om aflopend op tekst te kunnen sorteren."""

    __slots__ = ("s",)

    def __init__(self, s):
        self.s = s

    def __lt__(self, other):
        return self.s > getattr(other, "s", other)


class GroupItem(QTreeWidgetItem):
    """Sectiekop bij 'Groeperen op': niet selecteerbaar, vet, met aantal."""

    def __init__(self, label, count):
        super().__init__([f"▾ {label}", f"{count} items", "", ""])
        font = self.font(COL_NAME)
        font.setBold(True)
        self.setFont(COL_NAME, font)
        self.setFlags(Qt.ItemFlag.ItemIsEnabled)


class FileView(QWidget):
    """One file display (single view)."""

    path_changed = pyqtSignal(str)
    footer_changed = pyqtSignal(str)

    def _set_footer(self, text):
        self._last_footer = text
        self.footer_changed.emit(text)

    def __init__(self, parent=None, side="left"):
        super().__init__(parent)
        self.side = side  # paneel-zijde voor onafhankelijke weergave-instellingen
        self.current_path = os.path.expanduser("~")
        self.show_hidden = False
        self.history = []
        self.hist_pos = -1
        self.pinned = False
        self.pinned_path = None  # locatie waar de tab vastgepind is
        self.custom_title = None
        self.panel_ref = None
        self.extra_cols = []  # audio tag columns active for current dir
        self._size_map = {}   # dir path -> tree item (for folder sizes)
        self.selection_callback = None
        # Weergave-opties (uit Instellingen/Beeld, persistent)
        cfg = load_ui_settings()
        sfx = f"_{side}"  # per-paneel instellingen, met globale fallback
        self.thumbnails = bool(
            cfg.get(f"view_thumbnails{sfx}", cfg.get("view_thumbnails", False)))
        self.flat = bool(
            cfg.get(f"view_flat{sfx}", cfg.get("view_flat", False)))
        self.compact = bool(
            cfg.get(f"view_compact{sfx}", cfg.get("view_compact", False)))
        self.thumb_size = int(
            cfg.get(f"view_thumb_size{sfx}", cfg.get("view_thumb_size", 96)))
        self.thumb_info = bool(
            cfg.get(f"view_thumb_info{sfx}", cfg.get("view_thumb_info", True)))
        self.color_files = bool(
            cfg.get(f"view_colors{sfx}", cfg.get("view_colors", False)))
        self.group_mode = str(
            cfg.get(f"view_group{sfx}", cfg.get("view_group", "")) or "")
        self._thumb_cache = {}  # (path, mtime, size) -> QIcon
        self.remote = False      # pad op netwerk-/FUSE-mount (rclone e.d.)
        self._load_seq = 0
        self._last_footer = ""
        self._loader = None
        # Wachthond: waarschuw als een netwerkmap heel lang nodig heeft
        self._load_watch = QTimer(self)
        self._load_watch.setSingleShot(True)
        self._load_watch.timeout.connect(self._load_watchdog)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(2)

        nav_widget = QWidget()
        nav_widget.setObjectName("navRow")
        nav = QHBoxLayout(nav_widget)
        nav.setContentsMargins(4, 2, 4, 2)
        self.btn_back = QPushButton("◀")
        self.btn_back.setObjectName("navBtn")
        self.btn_up = QPushButton("⬆")
        self.btn_up.setObjectName("navBtn")
        self.btn_fwd = QPushButton("▶")
        self.btn_fwd.setObjectName("navBtn")
        self.btn_home = QPushButton("🏠")
        self.btn_home.setObjectName("navBtn")
        for b in (self.btn_back, self.btn_up, self.btn_fwd, self.btn_home):
            b.setFixedWidth(30)
        self.btn_up.setToolTip("Omhoog")
        self.path_bar = PathBar()
        self.path_bar.navigate.connect(self.load_path)
        # Weergave-knop (thumbnail-formaat en -info kiezen)
        self.btn_viewmode = QToolButton()
        self.btn_viewmode.setText("🖼")
        self.btn_viewmode.setToolTip(tr("Weergave kiezen (thumbnails, grootte, info)"))
        self.btn_viewmode.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        self.btn_viewmode.setFixedWidth(30)
        self._build_viewmode_menu()
        nav.addWidget(self.btn_back)
        nav.addWidget(self.btn_up)
        nav.addWidget(self.btn_fwd)
        nav.addWidget(self.path_bar, 1)
        nav.addWidget(self.btn_home)
        nav.addWidget(self.btn_viewmode)
        layout.addWidget(nav_widget)

        # Filter bar (hidden until toggled)
        self.filter_row = QWidget()
        frow = QHBoxLayout(self.filter_row)
        frow.setContentsMargins(4, 0, 4, 0)
        frow.addWidget(QLabel(tr("Filter:")))
        self.filter_edit = QLineEdit()
        self.filter_edit.setPlaceholderText(
            tr("Typ om te filteren op naam (leeg = alles tonen)"))
        self.filter_edit.textChanged.connect(self._apply_filter)
        clr = QPushButton("✕")
        clr.setFixedWidth(26)
        clr.setToolTip(tr("Filter wissen"))
        clr.clicked.connect(self.filter_edit.clear)
        frow.addWidget(self.filter_edit, 1)
        frow.addWidget(clr)
        self.filter_row.setVisible(False)
        layout.addWidget(self.filter_row)

        # Details-mode file list
        self.tree = QTreeWidget()
        self.tree.setColumnCount(4)
        self.tree.setHeaderLabels(["Naam", "Grootte", "Type", "Gewijzigd"])
        header = self.tree.header()
        # Interactive everywhere so user-set widths are kept as-is
        for c in range(4):
            header.setSectionResizeMode(c, QHeaderView.ResizeMode.Interactive)
        header.setDefaultSectionSize(120)
        header.setStretchLastSection(False)
        header.setSectionsClickable(True)
        header.setSortIndicator(COL_NAME, Qt.SortOrder.AscendingOrder)
        self.tree.setSortingEnabled(True)
        self.tree.setSelectionMode(QTreeWidget.SelectionMode.ExtendedSelection)
        self.tree.setSelectionBehavior(QTreeWidget.SelectionBehavior.SelectRows)
        self.tree.setRootIsDecorated(False)
        self.tree.setItemsExpandable(True)
        self.tree.setExpandsOnDoubleClick(False)
        self.tree.setAlternatingRowColors(True)
        self.tree.itemDoubleClicked.connect(self._on_double_click)
        self.tree.itemExpanded.connect(self._on_item_expanded)
        self.tree.itemClicked.connect(self._on_item_clicked)
        self.tree.itemChanged.connect(self._on_item_changed)
        self.tree.itemSelectionChanged.connect(self._on_selection_changed)
        self.tree.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self._last_click = (0.0, None)  # (time, item-ref) for slow-double-click
        self.tree.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.tree.customContextMenuRequested.connect(self._context_menu)
        layout.addWidget(self.tree, 1)

        self.footer = None  # footer text goes to the shared bottom bar

        self.btn_up.clicked.connect(self.go_up)
        self.btn_back.clicked.connect(self.go_back)
        self.btn_fwd.clicked.connect(self.go_forward)
        self.btn_home.clicked.connect(lambda: self.load_path(os.path.expanduser("~")))

        self.watcher = QFileSystemWatcher()
        self.watcher.directoryChanged.connect(lambda _p: self.refresh())
        self._apply_saved_col_widths()

        # Debounced saving of column widths while the user drags them
        self._col_timer = QTimer(self)
        self._col_timer.setSingleShot(True)
        self._col_timer.setInterval(400)
        self._col_timer.timeout.connect(self._save_col_widths)
        header.sectionResized.connect(lambda *_a: self._col_timer.start())

        self.apply_display_options()
        self.load_path(self.current_path)

    # ---------- weergave-knop (thumbnails: formaat + info) ----------
    def _build_viewmode_menu(self):
        menu = QMenu(self)
        cfg = load_ui_settings()
        sfx = f"_{self.side}"
        act_off = QAction(tr("Gewone lijst (geen thumbnails)"), menu)
        act_off.setCheckable(True)
        act_off.setChecked(not bool(
            cfg.get(f"view_thumbnails{sfx}",
                    cfg.get("view_thumbnails", False))))
        act_off.triggered.connect(
            lambda: self._set_global_view("view_thumbnails", False))
        menu.addAction(act_off)
        for label, size in ((tr("Kleine thumbnails (48 px)"), 48),
                            (tr("Middelgrote thumbnails (96 px)"), 96),
                            (tr("Grote thumbnails (160 px)"), 160)):
            act = QAction(label, menu)
            act.setCheckable(True)
            act.setChecked(bool(
                cfg.get(f"view_thumbnails{sfx}",
                        cfg.get("view_thumbnails", False)))
                and int(cfg.get(f"view_thumb_size{sfx}",
                                cfg.get("view_thumb_size", 96))) == size)
            act.triggered.connect(
                lambda _c=False, s=size: self._set_thumb_mode(s))
            menu.addAction(act)
        menu.addSeparator()
        act_info = QAction(tr("Info tonen (grootte, type, datum)"), menu)
        act_info.setCheckable(True)
        act_info.setChecked(bool(
            cfg.get(f"view_thumb_info{sfx}",
                    cfg.get("view_thumb_info", True))))
        act_info.toggled.connect(
            lambda on: self._set_global_view("view_thumb_info", on))
        menu.addAction(act_info)
        menu.addSeparator()
        act_tree = QAction(tr("Boomweergave (submappen uitklappen)"), menu)
        act_tree.setCheckable(True)
        act_tree.setChecked(bool(
            cfg.get(f"view_flat{sfx}", cfg.get("view_flat", False))))
        act_tree.toggled.connect(
            lambda on: self._set_global_view("view_flat", on))
        menu.addAction(act_tree)
        menu.addSeparator()
        act_compact = QAction(tr("Compacte modus"), menu)
        act_compact.setCheckable(True)
        act_compact.setChecked(bool(
            cfg.get(f"view_compact{sfx}", cfg.get("view_compact", False))))
        act_compact.toggled.connect(
            lambda on: self._set_global_view("view_compact", on))
        menu.addAction(act_compact)
        menu.addSeparator()
        act_color = QAction(tr("Kleurcodering van bestanden"), menu)
        act_color.setCheckable(True)
        act_color.setChecked(bool(
            cfg.get(f"view_colors{sfx}", cfg.get("view_colors", False))))
        act_color.toggled.connect(
            lambda on: self._set_global_view("view_colors", on))
        menu.addAction(act_color)
        gmenu = menu.addMenu(tr("Groeperen op"))
        group_opts = [
            ("", tr("Uit")),
            ("type", tr("Bestandstype")),
            ("date", tr("Datum (vandaag/gisteren/...)")),
            ("size", tr("Grootte-klasse")),
            ("sortcol", tr("Huidige sorteerkolom")),
        ]
        cur_group = str(
            cfg.get(f"view_group{sfx}", cfg.get("view_group", "")) or "")
        for val, label in group_opts:
            act_g = QAction(label, gmenu)
            act_g.setCheckable(True)
            act_g.setChecked(cur_group == val)
            act_g.triggered.connect(
                lambda _c=False, v=val: self._set_global_view("view_group", v))
            gmenu.addAction(act_g)
        self.btn_viewmode.setMenu(menu)

    def _set_thumb_mode(self, size):
        """Zet thumbnails aan op gevraagd formaat (of uit als al actief)."""
        cfg = load_ui_settings()
        if (cfg.get("view_thumbnails", False)
                and int(cfg.get("view_thumb_size", 96)) == size):
            self._set_global_view("view_thumbnails", False)
        else:
            self._set_thumb_size(size)

    def _set_global_view(self, key, value):
        win = self.window()
        if isinstance(win, MainWindow):
            win.set_display_options(key, value, side=self.side)
        else:
            # Standalone (tests): alleen lokaal toepassen
            data = load_ui_settings()
            data[f"{key}_{self.side}"] = bool(value)
            self._apply_view_setting(key, value)
            self.refresh()

    def _set_thumb_size(self, size):
        win = self.window()
        if isinstance(win, MainWindow):
            # Grootte kiezen zet thumbnails meteen aan (geen menu-vinkje nodig)
            win.set_display_options("view_thumbnails", True, side=self.side)
            win.set_display_options("view_thumb_size", size, side=self.side)
        else:
            data = load_ui_settings()
            data[f"view_thumb_size_{self.side}"] = int(size)
            data[f"view_thumbnails_{self.side}"] = True
            self._apply_view_setting("view_thumb_size", int(size))
            self._apply_view_setting("view_thumbnails", True)
            self.refresh()

    def _apply_view_setting(self, key, value):
        """Werk één weergave-instelling bij op deze view (zonder opslag)."""
        if key == "view_thumbnails":
            self.thumbnails = bool(value)
        elif key == "view_flat":
            self.flat = bool(value)
        elif key == "view_compact":
            self.compact = bool(value)
        elif key == "view_thumb_size":
            self.thumb_size = int(value)
            self._thumb_cache.clear()
        elif key == "view_thumb_info":
            self.thumb_info = bool(value)
        elif key == "view_colors":
            self.color_files = bool(value)
        elif key == "view_group":
            self.group_mode = str(value or "")
        self.apply_display_options()

    def _apply_saved_col_widths(self, side="left"):
        data = load_ui_settings()
        widths = data.get(f"col_widths_{side}")
        if isinstance(widths, list):
            for c, wd in enumerate(widths[: self.tree.columnCount()]):
                try:
                    self.tree.setColumnWidth(c, max(30, int(wd)))
                except (TypeError, ValueError):
                    continue

    # ---------- per-directory columns ----------
    def _dir_columns_for(self, path):
        """Extra columns for a path: inherited from the nearest configured
        ancestor directory (the directory itself counts too)."""
        mapping = load_ui_settings().get("dir_columns", {})
        if not mapping:
            return []
        p = path.rstrip("/")
        best_key = None
        best_val = None
        for k, val in mapping.items():
            kk = k.rstrip("/")
            if p == kk or p.startswith(kk + "/"):
                if best_key is None or len(kk) > len(best_key):
                    best_key, best_val = kk, val
        if best_key is None:
            return []
        if isinstance(best_val, dict):
            cols = best_val.get("cols", [])
            if p != best_key and not best_val.get("sub", True):
                return []  # explicitly limited to that folder only
        else:
            cols = best_val
        return [c for c in COLUMN_LABELS if c in cols]

    def _reload_dir_columns(self):
        """Load which extra (audio) columns are active for the current dir."""
        self.extra_cols = self._dir_columns_for(self.current_path)

    def _setup_columns(self):
        labels = ["Naam", "Grootte", "Type", "Gewijzigd"]
        labels += [COLUMN_LABELS[c] for c in self.extra_cols]
        sort_col = max(self.tree.header().sortIndicatorSection(), 0)
        sort_order = self.tree.header().sortIndicatorOrder()
        self.tree.setSortingEnabled(False)
        self.tree.clear()
        self.tree.setColumnCount(len(labels))
        self.tree.setHeaderLabels(labels)
        header = self.tree.header()
        for c in range(len(labels)):
            header.setSectionResizeMode(c, QHeaderView.ResizeMode.Interactive)
        header.setSortIndicator(sort_col if sort_col < len(labels) else COL_NAME,
                                sort_order)
        self.tree.setSortingEnabled(True)

    def _toggle_dir_column(self, key, on):
        data = load_ui_settings()
        mapping = data.setdefault("dir_columns", {})
        cur = mapping.get(self.current_path, {})
        cols = set(cur.get("cols", []) if isinstance(cur, dict) else cur)
        if on:
            cols.add(key)
        else:
            cols.discard(key)
        if cols:
            mapping[self.current_path] = {"cols": sorted(cols), "sub": True}
        else:
            mapping.pop(self.current_path, None)
        data["dir_columns"] = mapping
        win = self.window()
        if isinstance(win, MainWindow):
            win._write_settings(data)
        self.load_path(self.current_path)

    def _set_all_audio_columns(self, on):
        data = load_ui_settings()
        mapping = data.setdefault("dir_columns", {})
        if on:
            mapping[self.current_path] = {
                "cols": list(COLUMN_LABELS.keys()), "sub": True}
        else:
            mapping.pop(self.current_path, None)
        data["dir_columns"] = mapping
        win = self.window()
        if isinstance(win, MainWindow):
            win._write_settings(data)
        self.load_path(self.current_path)

    def _save_col_widths(self):
        win = self.window()
        if isinstance(win, MainWindow):
            win.save_settings()

    def shutdown(self):
        """Stop watching directories; called when this view is discarded."""
        self._load_watch.stop()
        if self._loader is not None:
            try:
                self._loader.scanned.disconnect(self._on_dir_scanned)
            except (TypeError, RuntimeError):
                pass
        try:
            self.watcher.directoryChanged.disconnect()
        except TypeError:
            pass
        try:
            dirs = self.watcher.directories()
            if dirs:
                self.watcher.removePaths(dirs)
        except RuntimeError:
            pass

    def load_path(self, path):
        try:
            self._load_path_guarded(path)
        except RuntimeError as e:
            # Widget was deleted (e.g. closed tab) while a signal arrived
            log_error(f"load_path op verwijderde view: {e}")

    def _load_path_guarded(self, path):
        path = os.path.abspath(path)
        # Guard tegen re-entrant loads (watcher events tijdens dialogs).
        # Bij een TRAGE netwerk-mount geldt de guard niet: een nieuw
        # verzoek VERVANGT de lopende lading (het oude resultaat wordt
        # toch genegeerd via de pad-vergelijking). Anders zou navigeren
        # tijdens een langzame load gewoon niks doen.
        if getattr(self, "_loading", False):
            if not getattr(self, "remote", False):
                log_error(f"load_path geblokkeerd (al aan het laden): {path}")
                return
            log_error(f"load_path vervangt lopende netwerk-lading: {path}")
        self._loading = True
        try:
            res = self._load_path_inner(path)
        except Exception as e:  # noqa: BLE001 - never leave UI half-updated silently
            self._set_footer(f"⚠ Fout bij openen van map: {path}")
            log_error(f"load_path faalde voor {path}", e)
            res = None
        finally:
            # Bij asynchroon laden (netwerkmap) blijft de guard staan tot
            # _finish_load/_on_dir_scanned klaar is.
            if res != "async":
                self._loading = False

    def _load_path_inner(self, path):
        # Voor netwerk-mounts géén synchrone isdir/stat in de UI-thread:
        # op een trage of dode rclone-verbinding zou dat alles blokkeren.
        if not self.remote and not os.path.isdir(path):
            if path.startswith("/mnt/lopus-nfs/") or path.startswith(
                    "/mnt/lopus-net/"):
                self._set_footer(
                    "⚠ Share niet verbonden — open hem via de zijbalk "
                    "(🌐 Netwerk ▸ 🔗 ...)")
            else:
                self._set_footer(f"⚠ Geen geldige map: {path}")
            return
        if self.watcher.directories():
            self.watcher.blockSignals(True)
            self.watcher.removePaths(self.watcher.directories())
        # Track navigation history
        if self.hist_pos < len(self.history) - 1:
            del self.history[self.hist_pos + 1 :]
        self.history.append(path)
        self.hist_pos = len(self.history) - 1
        self.current_path = path
        # Watcher alleen op lokale schijven: op netwerk-/FUSE-mounts
        # (rclone e.d.) blokkeert of crasht die.
        self.remote = is_remote_path(path)
        if not self.remote:
            self.watcher.addPath(path)
        self.watcher.blockSignals(False)
        self.path_bar.set_path(path)
        # Configure columns for THIS directory (audio tags etc.)
        self._reload_dir_columns()
        self._setup_columns()
        self._apply_saved_col_widths(
            getattr(self.panel_ref, "side", "left") if self.panel_ref else "left"
        )
        self.apply_display_options()

        self._load_seq += 1
        if self.remote:
            # Netwerk-mount: lezen kan lang duren -> achtergrondthread,
            # zodat de UI niet bevriest.
            self._set_footer(f"⏳ Bezig met laden (netwerk): {path} ...")
            self.tree.clear()
            loader = DirLoader(path, self.show_hidden, self)
            loader.scanned.connect(self._on_dir_scanned)
            self._loader = loader  # keep reference
            loader.finished.connect(loader.deleteLater)
            loader.start()
            self._load_watch.start(20_000)
            return "async"
        folders, files = scan_dir_entries(path, self.show_hidden)
        self._finish_load(path, folders, files)

    def _on_dir_scanned(self, path, folders, files):
        """Klaar met achtergrond-lezen van een netwerkmap."""
        self._load_watch.stop()
        if path != self.current_path:
            # verouderd resultaat: gebruiker is ondertussen verder
            # genavigeerd (die nieuwe lading is nog bezig — de guard
            # blijft dus staan tot die klaar is).
            return
        if folders is None:  # geen geldige map (bijv. dode mount)
            self._set_footer(f"⚠ Geen geldige (bereikbare) map: {path}")
            self.tree.clear()
            self._loading = False
            return
        self._finish_load(path, folders, files)

    def _load_watchdog(self):
        """20 s aan het laden op een netwerkmount: geef eerlijk terug dat
        de mount traag of mogelijk vastgelopen is."""
        if self._loading and self.remote:
            self._set_footer(
                "⚠ Nog steeds laden (netwerk, >20 s): de mount reageert "
                "traag of niet. Probeer Vernieuwen, of herstart de mount "
                "(bijv. rclone).")

    def _finish_load(self, path, folders, files):
        self._load_watch.stop()
        self._cat_colors = category_colors()  # kleurcodering per load
        self._last_entries = folders + files
        for e in self._last_entries:
            self._add_item(e)
        self.tree.sortItems(
            self.tree.header().sortIndicatorSection(),
            self.tree.header().sortIndicatorOrder(),
        )
        if self.group_mode:
            self._apply_grouping(self._last_entries)
        self._set_footer(
            f"{path}   |   {len(folders)} mappen, {len(files)} bestanden"
        )
        self.tree.viewport().update()
        if load_ui_settings().get("map_groottes") and not self.remote:
            self.compute_dir_sizes()
        self.path_changed.emit(path)
        self._loading = False
        self._update_free_space(path)

    def _update_free_space(self, path):
        """Vrije ruimte van het bestandssysteem van 'path' achteraan de
        onderbalk. Lokaal direct; op netwerk-mounts op de achtergrond."""
        # Onthoud welk pad we nú opvragen
        target_path = path

        def done(free):
            # Controleer of we inmiddels op een heel ander pad zitten in dit paneel
            if (not isinstance(free, int) or free < 0
                    or getattr(self, 'current_path', None) != target_path
                    or not os.path.isdir(target_path)):
                return

            mp = mountpoint_of(target_path)
            label = os.path.basename(mp.rstrip("/")) or "systeem"

            # Extra veiligheid: check of het actieve paneel nog steeds bij dit pad hoort
            if self.current_path != target_path:
                return

            self._set_footer(self._footer_text()
                             + f"    |    💾 {human_size(free)} vrij ({label})")

        if not self.remote:
            try:
                done(shutil.disk_usage(path).free)
            except OSError:
                pass
            return

        # Voor netwerk/remote: draai op de achtergrond met veilige referentie
        wk = FuncWorker(lambda p=path: shutil.disk_usage(p).free)
        wk.result_ready.connect(done)
        wk.finished.connect(wk.deleteLater)
        wk.start()
        self._fs_worker = wk  # referentie bewaren

    # ---------- groeperen op ----------
    def _group_label(self, e):
        """Groepsnaam voor een entry volgens self.group_mode."""
        mode = self.group_mode
        if mode == "type":
            if e["isdir"]:
                return "Mappen"
            return (os.path.splitext(e["name"])[1].lstrip(".").upper()
                    or "Zonder extensie")
        if mode == "date":
            age = time.time() - e["mtime"]
            day = 86400.0
            if age < day:
                return "Vandaag"
            if age < 2 * day:
                return "Gisteren"
            if age < 7 * day:
                return "Deze week"
            if age < 31 * day:
                return "Deze maand"
            return "Ouder"
        if mode == "size":
            if e["isdir"]:
                return "Mappen"
            s = e["size"] or 0
            if s < 1024 * 1024:
                return "Klein (< 1 MB)"
            if s < 10 * 1024 * 1024:
                return "1 – 10 MB"
            if s < 100 * 1024 * 1024:
                return "10 – 100 MB"
            if s < 1024 * 1024 * 1024:
                return "100 MB – 1 GB"
            return "Enorm (> 1 GB)"
        if mode == "sortcol":
            col = max(self.tree.header().sortIndicatorSection(), 0)
            if col == COL_TYPE and not e["isdir"]:
                return (os.path.splitext(e["name"])[1].lstrip(".").upper()
                        or "Zonder extensie")
            if col in (COL_SIZE,) and not e["isdir"]:
                return self._group_label({**e, "isdir": False})
            if col == COL_MODIFIED:
                return self._group_label_date(e)
            return e["name"][:1].upper()
        return ""

    def _group_label_date(self, e):
        saved = self.group_mode
        self.group_mode = "date"
        try:
            return self._group_label(e)
        finally:
            self.group_mode = saved

    def _entry_sort_key(self, e):
        col = max(self.tree.header().sortIndicatorSection(), 0)
        asc = self.tree.header().sortIndicatorOrder() == \
            Qt.SortOrder.AscendingOrder
        if col == COL_SIZE:
            key = (-1 if e["isdir"] else (e["size"] or 0))
        elif col == COL_MODIFIED:
            key = e["mtime"]
        else:
            key = e["name"].lower()
        if not asc:
            def rev(k):
                if isinstance(k, str):
                    return _ReverseStr(k)
                return -k
            key = rev(key)
        return ((0 if e["isdir"] else 1), key)

    def _apply_grouping(self, entries):
        """Bouw de lijst opnieuw op als gegroepeerde secties."""
        groups = {}
        for e in entries:
            label = self._group_label(e)
            groups.setdefault(label, []).append(e)
        order = {"date": ["Vandaag", "Gisteren", "Deze week", "Deze maand",
                          "Ouder"],
                 "size": ["Mappen", "Klein (< 1 MB)", "1 – 10 MB",
                          "10 – 100 MB", "100 MB – 1 GB", "Enorm (> 1 GB)"],
                 }.get(self.group_mode)
        labels = [l for l in (order or []) if l in groups]
        labels += sorted((l for l in groups if l not in labels),
                         key=str.lower)
        self.tree.setSortingEnabled(False)
        self.tree.clear()
        for label in labels:
            items_e = sorted(groups[label], key=self._entry_sort_key)
            header = GroupItem(label, len(items_e))
            self.tree.addTopLevelItem(header)
            for e in items_e:
                header.addChild(self._add_item(e, parent=header))
        self.tree.expandAll()

    def _footer_text(self):
        return self._last_footer

    def _generic_icon(self, isdir):
        """Generiek icoon zonder schijf-toegang (voor netwerk-mounts)."""
        if not hasattr(self, "_generic_icons"):
            style = self.style()
            self._generic_icons = {
                True: style.standardIcon(
                    style.StandardPixmap.SP_DirIcon),
                False: style.standardIcon(
                    style.StandardPixmap.SP_FileIcon),
            }
        return self._generic_icons[bool(isdir)]

    def _add_item(self, e, parent=None):
        item = SortableItem()
        name = e["name"]
        if e["isdir"]:
            name += "/"
            size_txt, ftype = "<DIR>", ("Snelkoppeling" if e["islink"] else "Map")
        else:
            size_txt = human_size(e["size"])
            ftype = (os.path.splitext(name)[1].lstrip(".").lower() or "bestand")[:10]
        item.setText(COL_NAME, name)
        # Op netwerk-mounts geen QFileIconProvider: die doet een stat per
        # bestand en blokkeert de UI bij trage/dode verbindingen.
        if self.remote:
            item.setIcon(COL_NAME, self._generic_icon(e["isdir"]))
        else:
            item.setIcon(COL_NAME, ICON_PROVIDER.icon(QFileInfo(e["full"])))
        item.setText(COL_SIZE, size_txt)
        # Raw size for numeric sorting (dirs get -1, they sort by group anyway)
        item.setData(COL_SIZE, Qt.ItemDataRole.UserRole + 1,
                     -1 if e["isdir"] else (e["size"] or 0))
        item.setText(COL_TYPE, ftype)
        item.setText(
            COL_MODIFIED, datetime.fromtimestamp(e["mtime"]).strftime("%d-%m-%Y %H:%M")
        )
        # Raw timestamp for chronological sorting (dd-mm-jjjj-tekst sorteerde
        # verkeerd: 31-03 kwam na 30-05 bij aflopend)
        item.setData(COL_MODIFIED, Qt.ItemDataRole.UserRole + 1, e["mtime"])
        item.setData(COL_NAME, Qt.ItemDataRole.UserRole, e)
        # Allow inline rename (slow double-click)
        item.setFlags(item.flags() | Qt.ItemFlag.ItemIsEditable)
        # Thumbnails: echte voorbeeldplaatjes voor afbeeldingen
        # (op netwerk-mounts niet: QPixmap zou het bestand via het netwerk
        # in de UI-thread lezen en de app bevriezen)
        if self.thumbnails and not e["isdir"] and not self.remote:
            ext = os.path.splitext(e["full"])[1].lower()
            if ext in IMAGE_EXTS and (e["size"] or 0) < 20 * 1024 * 1024:
                key = (e["full"], e["mtime"], self.thumb_size)
                ic = self._thumb_cache.get(key)
                if ic is None:
                    pm = QPixmap(e["full"])
                    if not pm.isNull():
                        pm = pm.scaled(
                            self.thumb_size, self.thumb_size,
                            Qt.AspectRatioMode.KeepAspectRatio,
                            Qt.TransformationMode.SmoothTransformation)
                        ic = QIcon(pm)
                    else:
                        ic = ICON_PROVIDER.icon(QFileInfo(e["full"]))
                    self._thumb_cache[key] = ic
                item.setIcon(COL_NAME, ic)
                item.setSizeHint(0, QSize(0, self.thumb_size + 8))
        # Extra (tag/media) columns
        if self.extra_cols:
            for i, key in enumerate(self.extra_cols):
                item.setText(4 + i, get_column_value(key, e["full"],
                                                     e["isdir"]))
        if e["islink"]:
            for c in range(self.tree.columnCount()):
                item.setForeground(c, QColor("#7aa7ff"))
        else:
            if self.color_files and not e["isdir"]:
                cat = file_category(
                    os.path.splitext(e["name"])[1].lower())
                if cat:
                    if not hasattr(self, "_cat_colors"):
                        self._cat_colors = category_colors()
                    col = self._cat_colors.get(cat)
                    if col is not None:
                        for c in range(self.tree.columnCount()):
                            item.setForeground(c, col)
            if e["isdir"]:
                font = item.font(COL_NAME)
                font.setBold(True)
                item.setFont(COL_NAME, font)
        # Boomweergave: mappen krijgen een dummy-kind zodat het uitklap-
        # pijltje (▶) zichtbaar is; de inhoud wordt lazy geladen.
        if self.flat and e["isdir"] and not e["islink"]:
            dummy = QTreeWidgetItem()
            dummy.setData(COL_NAME, Qt.ItemDataRole.UserRole, None)
            item.addChild(dummy)
        if parent is None:
            self.tree.addTopLevelItem(item)
        else:
            parent.addChild(item)
        return item

    def _list_dir_entries(self, path):
        """Map-inhoud als entry-dicts (gebruikt voor boomweergave-uitklap)."""
        out = []
        try:
            names = os.listdir(path)
        except OSError:
            return out
        for name in sorted(names, key=str.lower):
            if name.startswith(".") and not self.show_hidden:
                continue
            full = os.path.join(path, name)
            try:
                st = os.stat(full, follow_symlinks=False)
            except OSError:
                continue
            isdir = statmod.S_ISDIR(st.st_mode)
            out.append({
                "name": name,
                "full": full,
                "isdir": isdir,
                "islink": os.path.islink(full),
                "size": None if isdir else st.st_size,
                "mtime": st.st_mtime,
            })
        return out

    def _populate_children(self, parent_item):
        """Laad de inhoud van een uitgeklapte map in de boomweergave."""
        e = parent_item.data(COL_NAME, Qt.ItemDataRole.UserRole)
        if not e or not e["isdir"]:
            return
        # Alleen vullen als er nog een dummy-kind in zit
        if not (parent_item.childCount() == 1
                and parent_item.child(0).data(
                    COL_NAME, Qt.ItemDataRole.UserRole) is None):
            return
        parent_item.takeChildren()
        entries = self._list_dir_entries(e["full"])
        for ce in entries:
            self._add_item(ce, parent=parent_item)
        parent_item.sortChildren(
            self.tree.header().sortIndicatorSection(),
            self.tree.header().sortIndicatorOrder(),
        )

    def _on_item_expanded(self, item):
        try:
            self._populate_children(item)
        except Exception as ex:  # noqa: BLE001
            log_error("boomweergave uitklappen faalde", ex)

    def refresh(self):
        self.load_path(self.current_path)

    def apply_display_options(self):
        """Pas thumbnails/compact/boomweergave instellingen toe op deze lijst."""
        # Boomweergave (flat): mappen uitklapbaar met pijltjes
        self.tree.setRootIsDecorated(self.flat)
        if self.thumbnails:
            self.tree.setIconSize(QSize(self.thumb_size, self.thumb_size))
            self.tree.setStyleSheet("")
        elif self.compact:
            self.tree.setIconSize(QSize(14, 14)) # Maak icoontjes eventueel iets kleiner zodat het past
            self.tree.setStyleSheet("QTreeView::item { height: 16px; padding: 0px; margin: 0px; }")
        else:
            self.tree.setIconSize(QSize(22, 22))
            self.tree.setStyleSheet("")
        # Info-kolommen: bij thumbnails zonder info alleen namen tonen
        hide_info = self.thumbnails and not self.thumb_info
        header = self.tree.header()
        for c in range(1, self.tree.columnCount()):
            header.setSectionHidden(c, hide_info)

    def set_filter_visible(self, visible):
        self.filter_row.setVisible(visible)
        if visible:
            self.filter_edit.setFocus()
        else:
            self.filter_edit.clear()

    def _apply_filter(self, text=None):
        needle = self.filter_edit.text().strip().lower()

        def hide_recursive(item):
            match = (not needle
                     or needle in item.text(COL_NAME).lower())
            item.setHidden(not match)
            for i in range(item.childCount()):
                child_visible = hide_recursive(item.child(i))
                if child_visible:
                    item.setHidden(False)
            return not item.isHidden()

        for i in range(self.tree.topLevelItemCount()):
            hide_recursive(self.tree.topLevelItem(i))

    def _on_selection_changed(self):
        cb = self.selection_callback
        if cb is None and self.panel_ref is not None:
            # Dynamic lookup: initial tabs were created before the callback
            # was attached to the panel.
            cb = getattr(self.panel_ref, "selection_callback", None)
        if cb:
            try:
                cb(self)
            except RuntimeError:
                pass

    # ---------- folder size / column chooser helpers ----------
    def _enabled_type_columns(self):
        """Union of columns enabled per bestandstype in Instellingen."""
        cfg = load_ui_settings().get("type_columns", {})
        enabled = set()
        for group, keys in cfg.items():
            enabled.update(keys)
        if not enabled:
            for keys in DEFAULT_TYPE_COLUMNS.values():
                enabled.update(keys)
        return [k for k, _lbl, _groups in COLUMN_LIBRARY if k in enabled]

    def _fill_column_menu(self, col_menu):
        """Fill the per-directory column chooser submenu (audio tags etc.)."""
        enabled = self._enabled_type_columns()
        current = set(self.extra_cols)
        for key, label, groups in COLUMN_LIBRARY:
            if key not in enabled:
                continue
            act = col_menu.addAction(COLUMN_LABELS[key])
            act.setCheckable(True)
            act.setChecked(key in current)
            act.toggled.connect(
                lambda on, k=key: self._toggle_dir_column(k, on)
            )
        col_menu.addSeparator()
        all_on = set(self.extra_cols) >= set(enabled) and bool(enabled)
        act_all = col_menu.addAction(
            "Alle kolommen uit" if all_on else "Alle beschikbare kolommen aan"
        )
        act_all.triggered.connect(
            lambda: self._set_all_audio_columns(not all_on))

    # ---------- folder sizes ----------
    def compute_dir_sizes(self):
        paths = []
        self._size_map.clear()
        for i in range(self.tree.topLevelItemCount()):
            it = self.tree.topLevelItem(i)
            e = it.data(COL_NAME, Qt.ItemDataRole.UserRole)
            if e and e["isdir"]:
                self._size_map[e["full"]] = it
                paths.append(e["full"])
        if not paths:
            return
        self._size_worker = DirSizeWorker(paths)
        self._size_worker.size_ready.connect(self._dir_size_ready)
        self._size_worker.start()

    def _dir_size_ready(self, full, total):
        it = self._size_map.get(full)
        if it is None:
            return
        try:
            it.setText(COL_SIZE, human_size(total))
            it.setData(COL_SIZE, Qt.ItemDataRole.UserRole + 1, total)
        except RuntimeError:
            pass

    def go_up(self):
        parent = os.path.dirname(self.current_path)
        if parent != self.current_path:
            self.load_path(parent)

    def go_back(self):
        if self.hist_pos > 0:
            self.hist_pos -= 1
            self.load_path(self.history[self.hist_pos])

    def go_forward(self):
        if self.hist_pos < len(self.history) - 1:
            self.hist_pos += 1
            self.load_path(self.history[self.hist_pos])

    def _on_item_clicked(self, item, col):
        """Slow double-click (two clicks with a short pause) = inline rename."""
        import time as _time

        if col != COL_NAME:
            return
        now = _time.monotonic()
        last_t, last_item = self._last_click
        self._last_click = (now, item)
        if last_item is None or last_item is not item:
            return
        dt = now - last_t
        from PyQt6.QtGui import QDoubleValidator  # noqa: F401 (placeholder)

        dbl_interval = QApplication.doubleClickInterval() / 1000.0
        if dbl_interval < dt <= 1.2:
            e = item.data(COL_NAME, Qt.ItemDataRole.UserRole)
            if e and not e["islink"]:
                # Uitstellen: editItem direct vanuit de click-handler wordt
                # door de nog komende mouse-release meteen weer afgebroken.
                QTimer.singleShot(
                    0, lambda it=item: (
                        self.tree.editItem(it, COL_NAME)
                        if it is not None
                        and self.tree.indexOfTopLevelItem(it) >= 0
                        else None
                    )
                )

    def _on_item_changed(self, item, col):
        """Commit an inline rename to disk (deferred out of the signal)."""
        if col != COL_NAME or getattr(self, "_loading", False):
            return
        e = item.data(COL_NAME, Qt.ItemDataRole.UserRole)
        if e is None:
            return
        old_name = e["name"]
        new_name = item.text(col).strip().rstrip("/")
        display = old_name + ("/" if e["isdir"] else "")
        # Restore display text first, guarded so this handler doesn't recurse
        self._loading = True
        try:
            item.setText(col, display)
        finally:
            self._loading = False
        if not new_name or new_name == old_name.rstrip("/"):
            return
        src = e["full"]
        dst = os.path.join(os.path.dirname(e["full"]), new_name)
        # Do the actual rename after the signal has finished
        QTimer.singleShot(0, lambda: self._commit_rename(src, dst))

    def _commit_rename(self, src, dst):
        if os.path.exists(dst):
            QMessageBox.warning(self, APP_NAME, f"Bestaat al:\n{dst}")
            self.refresh()
            return
        try:
            os.rename(src, dst)
        except OSError as ex:
            QMessageBox.warning(self, APP_NAME, str(ex))
        self.refresh()

    def _on_double_click(self, item, _col):
        try:
            e = item.data(COL_NAME, Qt.ItemDataRole.UserRole)
            if e is None:
                return
            if e["isdir"] and not e["islink"]:
                target = e["full"]
                # Defer the reload until after the double-click event has
                # finished; rebuilding the tree inside the mouse handler
                # prevents Qt from repainting (stale/black rows).
                QTimer.singleShot(0, lambda: self.load_path(target))
            else:
                subprocess.Popen(["xdg-open", e["full"]])
        except Exception as e:  # noqa: BLE001
            self._set_footer("⚠ Kon item niet openen")
            log_error(f"_on_double_click faalde voor {item.text(0)}", e)

    def selected_paths(self):
        return [
            i.data(COL_NAME, Qt.ItemDataRole.UserRole)["full"]
            for i in self.tree.selectedItems()
            if i.data(COL_NAME, Qt.ItemDataRole.UserRole)
        ]

    def _context_menu(self, pos):
        try:
            self._context_menu_inner(pos)
        except Exception as e:  # noqa: BLE001
            log_error("contextmenu faalde", e)

    def _context_menu_inner(self, pos):
        menu = QMenu(self)
        sel = self.tree.selectedItems()
        single_file = None
        submenu = None
        paths_for_copy = []
        for it in sel:
            e = it.data(COL_NAME, Qt.ItemDataRole.UserRole)
            if e:
                paths_for_copy.append(e["full"])
        if len(sel) == 1:
            e = sel[0].data(COL_NAME, Qt.ItemDataRole.UserRole)
            if e and not e["isdir"]:
                single_file = e["full"]

        act_open = menu.addAction(tr("Openen"))
        act_root = menu.addAction(tr("🔓 Openen als root (beheerder)"))
        act_root_file = None
        if single_file:
            act_root_file = menu.addAction(tr("🔓 Bewerken als root"))
        act_archive = None
        if single_file and is_archive(single_file):
            act_archive = menu.addAction(tr("📦 Openen als archief..."))
        act_term = menu.addAction(tr("🖥 Terminal hier openen"))
        mount_act = None
        unmount_act = None
        if single_file:
            submenu = self._build_open_with_menu(menu, single_file)
            ext = os.path.splitext(single_file)[1].lower()
            if ext in MOUNTABLE_EXTS and shutil.which("udisksctl"):
                if single_file in _mounted_images:
                    unmount_act = menu.addAction(tr("📤 Ontkoppelen (unmount)"))
                else:
                    mount_act = menu.addAction(tr("📥 Koppelen als schijf (mount)"))
        menu.addSeparator()
        act_new_file = menu.addAction(tr("Nieuw bestand..."))
        act_new_folder = menu.addAction(tr("Nieuwe map...") + "  (F7)")
        single_dir = None
        if len(sel) == 1:
            e0 = sel[0].data(COL_NAME, Qt.ItemDataRole.UserRole)
            if e0 and e0["isdir"]:
                single_dir = e0["full"]
        act_tags = None
        if mutagen is not None:
            act_tags = menu.addAction(tr("🎵 Tag-editor..."))
            act_tags.setData(single_file or single_dir or self.current_path)
        if act_tags is not None:
            act_ren_mass = menu.addAction(tr("✏ Massaal hernoemen..."))
            act_ren_mass.triggered.connect(
                lambda _checked=False, p=(single_dir or self.current_path):
                    win.open_renamer(path=p))
        menu.addSeparator()
        act_copy_to = menu.addAction(tr("Kopiëren naar ander venster") + "  (F5)")
        act_move_to = menu.addAction(tr("Verplaatsen naar ander venster") + "  (F6)")
        act_rename = menu.addAction(tr("Hernoemen...") + "  (F2)")
        act_delete = menu.addAction(tr("Naar prullenbak") + "  (Del)")
        act_pack = None
        if paths_for_copy:
            act_pack = menu.addAction(tr("📦 Inpakken als archief..."))
        menu.addSeparator()
        target = single_file or single_dir or self.current_path
        beheer = menu.addMenu(tr("Beheer"))
        exec_act = None
        if single_file:
            if file_is_executable(single_file):
                exec_act = beheer.addAction(tr("⚙ Uitvoerbaar verwijderen (−x)"))
            else:
                exec_act = beheer.addAction(tr("⚙ Uitvoerbaar maken (+x)"))
        chmod_act = None
        if single_file or single_dir:
            chmod_act = beheer.addAction(tr("🔑 Rechten wijzigen (octaal)..."))
        share_act = None
        if single_dir:
            share_act = beheer.addAction(tr("🔗 Map delen..."))
        menu.addSeparator()
        act_ccopy = menu.addAction(tr("Kopiëren") + "\tCtrl+C")
        act_ccut = menu.addAction(tr("Knippen") + "\tCtrl+X")
        act_cpaste = menu.addAction(tr("Plakken") + "\tCtrl+V")
        name_menu = menu.addMenu(tr("Naam / pad kopiëren"))
        act_cname = name_menu.addAction(tr("Bestandsna(a)m(en) kopiëren"))
        act_cpath = name_menu.addAction(tr("Volledig(e) pad(en) kopiëren"))
        act_cname.setEnabled(bool(paths_for_copy))
        act_cpath.setEnabled(bool(paths_for_copy))
        menu.addSeparator()
        act_refresh = menu.addAction(tr("Vernieuwen"))
        act_props = menu.addAction(tr("Eigenschappen..."))

        chosen = menu.exec(self.tree.viewport().mapToGlobal(pos))
        if chosen is None:
            return
        win = self.window()
        if submenu is not None and chosen in self._open_with_actions:
            return  # handled via submenu trigger
        if act_tags is not None and chosen == act_tags:
            tpath = act_tags.data()
            if not isinstance(tpath, str) or not tpath:
                tpath = self.current_path
            win.open_tag_editor(path=tpath, force=True)
            return
        if chosen == act_open:
            cur = self.tree.currentItem()
            if cur:
                self._on_double_click(cur, 0)
        elif chosen == act_root:
            target_dir = single_dir or self.current_path
            err = open_root_lopus(target_dir)
            if err:
                QMessageBox.warning(self, APP_NAME, err)
        elif act_root_file is not None and chosen == act_root_file:
            err = open_file_as_root(single_file)
            if err:
                QMessageBox.warning(self, APP_NAME, err)
        elif mount_act is not None and chosen == mount_act:
            mp, err = mount_disk_image(single_file)
            if err:
                QMessageBox.warning(self, "Koppelen", err)
            else:
                QMessageBox.information(
                    self, "Koppelen", f"Gekoppeld op:\n{mp}"
                )
                win.dir_tree.reload()
                self.refresh()
        elif unmount_act is not None and chosen == unmount_act:
            err = unmount_disk_image(single_file)
            if err:
                QMessageBox.warning(self, "Ontkoppelen", err)
            else:
                win.dir_tree.reload()
                self.refresh()
        elif chosen == act_new_file:
            if new_empty_file(self.current_path):
                self.refresh()
        elif chosen == act_new_folder:
            self.new_folder()
        elif chosen == act_copy_to:
            win.transfer_selected(move=False)
        elif chosen == act_move_to:
            win.transfer_selected(move=True)
        elif chosen == act_rename:
            self.rename_selected()
        elif chosen == act_delete:
            win.delete_selected()
        elif act_archive is not None and chosen == act_archive:
            win.open_archive_dialog(single_file)
        elif act_pack is not None and chosen == act_pack:
            win.create_archive_selected()
        elif chosen == act_ccopy:
            win.clipboard_copy()
        elif chosen == act_ccut:
            win.clipboard_cut()
        elif chosen == act_cpaste:
            win.clipboard_paste()
        elif chosen == act_refresh:
            self.refresh()
        elif chosen == act_term:
            target = single_file or self.current_path
            err = open_terminal_in(target)
            if err:
                QMessageBox.warning(self, APP_NAME, err)
        elif exec_act is not None and chosen == exec_act:
            try:
                set_executable(single_file, not file_is_executable(single_file))
                self.refresh()
            except OSError as ex:
                QMessageBox.warning(self, APP_NAME, str(ex))
        elif share_act is not None and chosen == share_act:
            self._share_folder_dialog(single_dir)
        elif chosen == act_props:
            if len(paths_for_copy) > 1:
                show_properties_dialog_multi(self, paths_for_copy)
            else:
                show_properties_dialog(self, target)
        elif chmod_act is not None and chosen == chmod_act:
            ch_target = single_file or single_dir
            dlg = PermissionsDialog(ch_target, self)
            if dlg.exec():
                errors = dlg.apply()
                if errors:
                    QMessageBox.warning(self, "Rechten",
                                        "Niet gelukt:\n" + "\n".join(errors))
                self.refresh()
        elif chosen == act_cname:
            names = [os.path.basename(p.rstrip("/")) for p in paths_for_copy]
            QApplication.clipboard().setText("\n".join(names))
        elif chosen == act_cpath:
            QApplication.clipboard().setText("\n".join(paths_for_copy))
        elif exec_act is not None and chosen == exec_act:
            try:
                make_exec = not file_is_executable(single_file)
                set_executable(single_file, make_exec)
                self.refresh()
            except OSError as ex:
                QMessageBox.warning(self, APP_NAME, str(ex))
        elif share_act is not None and chosen == share_act:
            self._share_folder_dialog(single_dir)
        elif chosen == act_props:
            if len(paths_for_copy) > 1:
                show_properties_dialog_multi(self, paths_for_copy)
            else:
                show_properties_dialog(self, props_target)

    def _build_open_with_menu(self, parent_menu, file_path):
        """Build 'Openen met' submenu for a single file. Returns the submenu."""
        mime = get_mime_type(file_path)
        default, apps = list_apps_for_mime(mime)
        submenu = parent_menu.addMenu(f"Openen met ({mime})")
        self._open_with_actions = set()

        def add_app(desktop_id, suffix=""):
            act = QAction(desktop_app_name(desktop_id) + suffix, submenu)
            act.triggered.connect(
                lambda _c=False, d=desktop_id: open_with_app(d, [file_path])
            )
            submenu.addAction(act)
            self._open_with_actions.add(act)

        # Default app first (marked), so it is directly clickable
        if default:
            add_app(default, "  [standaard]")
        for app_id in apps[:15]:
            add_app(app_id)

        if not default and not apps:
            disabled = submenu.addAction(tr("(geen applicaties gevonden)"))
            disabled.setEnabled(False)

        submenu.addSeparator()
        act_default = submenu.addAction(tr("Als standaard instellen..."))
        act_other = submenu.addAction(tr("Andere toepassing..."))

        def choose_and_set_default():
            choices = ([default] if default else []) + apps
            names = [desktop_app_name(a) for a in choices]
            name, ok = QInputDialog.getItem(
                self,
                "Als standaard instellen",
                f"Standaardapplicatie voor {mime}:",
                names or ["(geen)"],
                0,
                False,
            )
            if ok and name != "(geen)" and name in names:
                set_default_app(mime, choices[names.index(name)])

        def choose_other():
            all_ids = []
            for base in (
                os.path.expanduser("~/.local/share/applications"),
                "/usr/share/applications",
                "/usr/local/share/applications",
            ):
                try:
                    for fn in os.listdir(base):
                        if fn.endswith(".desktop") and fn not in all_ids:
                            all_ids.append(fn)
                except OSError:
                    continue
            all_ids.sort(key=desktop_app_name)
            names = [desktop_app_name(a) for a in all_ids]
            name, ok = QInputDialog.getItem(
                self, "Andere toepassing", "Openen met:", names, 0, False
            )
            if ok:
                desktop_id = all_ids[names.index(name)]
                open_with_app(desktop_id, [file_path])
                r = QMessageBox.question(
                    self,
                    "Standaard",
                    f"'{name}' instellen als standaard voor {mime}?",
                )
                if r == QMessageBox.StandardButton.Yes:
                    set_default_app(mime, desktop_id)

        act_default.triggered.connect(choose_and_set_default)
        act_other.triggered.connect(choose_other)
        return submenu

    def new_folder(self):
        name, ok = QInputDialog.getText(self, "Nieuwe map", "Mapnaam:")
        if ok and name.strip():
            try:
                os.mkdir(os.path.join(self.current_path, name.strip()))
                self.refresh()
            except OSError as e:
                QMessageBox.warning(self, APP_NAME, str(e))

    def rename_selected(self):
        items = [i for i in self.tree.selectedItems()
                 if i.data(COL_NAME, Qt.ItemDataRole.UserRole)]
        if not items:
            return
        e = items[0].data(COL_NAME, Qt.ItemDataRole.UserRole)
        new_name, ok = QInputDialog.getText(
            self, "Hernoemen", "Nieuwe naam:", text=e["name"]
        )
        if ok and new_name.strip() and new_name != e["name"]:
            try:
                os.rename(e["full"], os.path.join(
                    os.path.dirname(e["full"]), new_name.strip()))
                self.refresh()
            except OSError as ex:
                QMessageBox.warning(self, APP_NAME, str(ex))

    # ---------- sharing ----------
    def _share_folder_dialog(self, directory):
        dlg = QDialog(self)
        dlg.setWindowTitle(f"Map delen — {os.path.basename(directory)}")
        dlg.resize(470, 320)
        lay = QVBoxLayout(dlg)
        lay.addWidget(QLabel(f"Map: {directory}"))

        status = QLabel("")
        status.setWordWrap(True)
        lay.addWidget(status)

        samba_btn = QPushButton("Via Samba (netwerkshare)")
        if not samba_available():
            samba_btn.setEnabled(False)
            samba_btn.setToolTip(tr("'net' (samba) niet gevonden op dit systeem"))
        lay.addWidget(samba_btn)
        unshare_btn = QPushButton(tr("Delen stoppen (Samba)"))
        unshare_btn.setVisible(False)
        lay.addWidget(unshare_btn)

        http_btn = QPushButton("Via webserver (http-link)")
        lay.addWidget(http_btn)
        stop_btn = QPushButton("Webserver stoppen")
        stop_btn.setEnabled(D_HTTP["server"] is not None)
        lay.addWidget(stop_btn)

        close_btn = QPushButton(tr("Sluiten"))
        close_btn.clicked.connect(dlg.close)
        lay.addWidget(close_btn)

        lay.addWidget(QLabel("Alle Samba-shares op deze pc:"))
        self.all_list = QListWidget()
        self.all_list.setToolTip(
            tr("Selecteer een share en klik op 'Geselecteerde share stoppen'"))
        lay.addWidget(self.all_list, 1)
        unshare_all_btn = QPushButton(tr("Geselecteerde share stoppen"))
        lay.addWidget(unshare_all_btn)

        def refresh_status():
            shares = samba_shares_for(directory)
            if shares:
                status.setText(
                    "📌 Deze map is NU gedeeld (Samba) als: "
                    + ", ".join(shares))
                unshare_btn.setVisible(True)
            else:
                status.setText(tr("Deze map is momenteel niet gedeeld (Samba)."))
                unshare_btn.setVisible(False)
            stop_btn.setEnabled(D_HTTP["server"] is not None)
            # Vul het overzicht met ALLE shares op deze pc
            self.all_list.clear()
            for name, spath in samba_all_shares():
                self.all_list.addItem(f"{name}  →  {spath}")

        def stop_selected_share():
            item = self.all_list.currentItem()
            if not item:
                return
            name = item.text().split("→")[0].strip()
            samba_share_delete(name)
            refresh_status()

        unshare_all_btn.clicked.connect(stop_selected_share)

        def do_samba():
            name, err = samba_share_add(directory)
            if err:
                status.setText(f"Samba mislukt: {err}")
            refresh_status()

        def do_unshare():
            for name in samba_shares_for(directory):
                samba_share_delete(name)
            refresh_status()

        def do_http():
            url = http_share_start(directory)
            if url.startswith("http"):
                QApplication.clipboard().setText(url)
                status.setText(
                    f"Delen via webserver gestart:\n{url}\n"
                    "(link is naar het klembord gekopieerd — de link deelt "
                    "de map die het laatst is gedeeld)")
            else:
                status.setText(url)
            refresh_status()

        samba_btn.clicked.connect(do_samba)
        unshare_btn.clicked.connect(do_unshare)
        http_btn.clicked.connect(do_http)
        stop_btn.clicked.connect(
            lambda: (http_share_stop(), refresh_status()))
        refresh_status()
        dlg.exec()


class TabDragBar(QTabBar):
    """Tabbalk die tabbladen ook naar het ándere paneel kan slepen."""

    MIME = "application/x-lopus-tab"

    def __init__(self, parent=None):
        super().__init__(parent)
        self.panel = None
        self._press_index = -1
        self._press_pos = None
        self.setAcceptDrops(True)
        self.setMovable(True)

    # ---- slepen naar ander paneel ----
    def mousePressEvent(self, e):
        if e.button() == Qt.MouseButton.LeftButton:
            self._press_pos = e.position().toPoint()
            self._press_index = self.tabAt(e.position().toPoint())
            # Klik op de tabstrook maakt dit paneel actief (onderbalk volgt)
            if self._press_index >= 0 and self.panel is not None:
                self.panel._become_active()
        else:
            self._press_index = -1
        super().mousePressEvent(e)

    def mouseMoveEvent(self, e):
        if (self._press_index >= 0 and self.panel is not None
                and (e.buttons() & Qt.MouseButton.LeftButton)
                and (e.position().toPoint() - self._press_pos
                     ).manhattanLength() > 25
                and not self.rect().contains(e.position().toPoint())):
            self._start_drag(self._press_index)
            self._press_index = -1
            return
        super().mouseMoveEvent(e)

    def _start_drag(self, index):
        drag = QDrag(self)
        mime = QMimeData()
        mime.setData(self.MIME,
                     f"{self.panel.side}|{index}".encode())
        drag.setMimeData(mime)
        drag.exec(Qt.DropAction.MoveAction)

    # ---- accepteren van gesleepte tabbladen ----
    def dragEnterEvent(self, e):
        if e.source() is not self and e.mimeData().hasFormat(self.MIME):
            e.acceptProposedAction()
        else:
            e.ignore()

    def dragMoveEvent(self, e):
        self.dragEnterEvent(e)

    def dropEvent(self, e):
        if self.panel is None or e.source() is self:
            return e.ignore()
        try:
            side, index = bytes(
                e.mimeData().data(self.MIME)).decode().split("|")
            index = int(index)
        except (ValueError, UnicodeDecodeError):
            return e.ignore()
        win = self.window()
        src = win.left if side == "left" else win.right
        if src is not self.panel and 0 <= index < src.tabs.count():
            self.panel.adopt_tab(src, index,
                                 self.tabAt(e.position().toPoint()))
            e.acceptProposedAction()


class FilePanel(QWidget):
    """A pane holding multiple tabs, each with its own FileView."""

    path_changed = pyqtSignal(str)
    footer_changed = pyqtSignal(str)

    def __init__(self, parent=None, side="left"):
        super().__init__(parent)
        self.side = side
        self.change_callback = None  # set by MainWindow to trigger saving
        self.selection_callback = None
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        self.tabs = QTabWidget()
        self.tabs.setDocumentMode(True)
        bar = TabDragBar()
        bar.panel = self
        self.tabs.setTabBar(bar)
        self.tabs.setMovable(True)
        # Never truncate tab titles: tabs grow with their text
        self.tabs.tabBar().setElideMode(Qt.TextElideMode.ElideNone)
        self.tabs.setUsesScrollButtons(True)
        bar.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        bar.customContextMenuRequested.connect(self._tab_menu)
        self.tabs.currentChanged.connect(self._on_tab_changed)
        layout.addWidget(self.tabs)

        self.add_tab(os.path.expanduser("~"))

    # ---------- active view ----------
    def view(self):
        return self.tabs.currentWidget()

    def _on_tab_changed(self, _i):
        if self.view() is not None:
            self.path_changed.emit(self.current_path)
            # Onderbalk direct de footer van de nieuwe tab laten tonen
            ft = getattr(self.view(), "_last_footer", "")
            if ft:
                self.footer_changed.emit(ft)
            self._become_active()

    def _become_active(self):
        """Markeer dit paneel als actief (klik op tab/tabstrook), zodat de
        onderbalk deze kant op wijst — ook zonder toetsenbordfocus."""
        win = self.window()
        if isinstance(win, MainWindow) and win._active_panel is not self:
            win._active_panel = self
            ft = (getattr(self.view(), "_last_footer", "")
                  or win._panel_footers.get(self, ""))
            win.set_status(ft or "")

    @property
    def current_path(self):
        v = self.view()
        return v.current_path if v is not None else os.path.expanduser("~")

    def selected_paths(self):
        return self.view().selected_paths()

    def refresh(self):
        for i in range(self.tabs.count()):
            self.tabs.widget(i).refresh()

    def set_filter_visible(self, visible):
        for i in range(self.tabs.count()):
            self.tabs.widget(i).set_filter_visible(visible)

    def set_show_hidden(self, show):
        for i in range(self.tabs.count()):
            w = self.tabs.widget(i)
            w.show_hidden = show
            w.refresh()

    @property
    def show_hidden(self):
        return self.view().show_hidden

    # ---------- tabs ----------
    def add_tab(self, path=None, index=None, pinned=False, title=None):
        v = FileView(side=self.side)
        v.panel_ref = self
        v.selection_callback = self.selection_callback
        v.footer_changed.connect(self.footer_changed)
        v._apply_saved_col_widths(self.side)
        v.pinned = pinned
        v.pinned_path = path if pinned else None
        v.custom_title = title
        if path:
            v.load_path(path)
        if index is None:
            idx = self.tabs.addTab(v, self._title(v))
        else:
            idx = self.tabs.insertTab(index, v, self._title(v))
        self.tabs.setCurrentIndex(idx)
        v._title_cb = lambda _p, t=v: self._set_title(t)
        v.path_changed.connect(v._title_cb)
        self._notify_change()
        return v

    def adopt_tab(self, src_panel, index, target_index=None):
        """Neem een tabblad van het andere paneel over (slepen/menu)."""
        if src_panel is self or index < 0 or index >= src_panel.tabs.count():
            return
        v = src_panel.tabs.widget(index)
        title = src_panel.tabs.tabText(index)
        # Signaal-verbindingen naar het oude paneel losmaken
        try:
            v.footer_changed.disconnect(src_panel.footer_changed)
        except TypeError:
            pass
        try:
            v.path_changed.disconnect(v._title_cb)
        except (TypeError, KeyError):
            pass
        src_panel.tabs.removeTab(index)
        # Nieuwe verbindingen naar dit paneel
        v.panel_ref = self
        v.selection_callback = self.selection_callback
        v.footer_changed.connect(self.footer_changed)
        v._title_cb = lambda _p, t=v: self._set_title(t)
        v.path_changed.connect(v._title_cb)
        idx = (self.tabs.insertTab(target_index, v, title)
               if target_index is not None and target_index >= 0
               else self.tabs.addTab(v, title))
        self.tabs.setCurrentIndex(idx)
        v._apply_saved_col_widths(self.side)
        # Laat het bronpaneel niet leeg achter
        if src_panel.tabs.count() == 0:
            src_panel.add_tab(os.path.expanduser("~"))
        src_panel._notify_change()
        self._notify_change()

    @staticmethod
    def _title(view):
        marker = "📌 " if view.pinned else ""
        if view.custom_title:
            return marker + view.custom_title
        # Vastgepinde tab toont de pin-locatie, ook als je tijdelijk
        # elders navigeert.
        base = (view.pinned_path or view.current_path)
        name = os.path.basename(base.rstrip("/")) or "/"
        return marker + (name or "/")

    def _tab_index(self, view):
        return self.tabs.indexOf(view)

    def _set_title(self, view):
        idx = self._tab_index(view)
        if idx >= 0:
            self.tabs.setTabText(idx, self._title(view))

    def close_tab(self, index):
        if self.tabs.count() <= 1:
            return
        w = self.tabs.widget(index)
        if w:
            w.shutdown()
        self.tabs.removeTab(index)
        w.deleteLater()
        self._notify_change()

    def _notify_change(self):
        if self.change_callback:
            self.change_callback()

    # ---------- tab context menu ----------
    def _tab_menu(self, pos):
        bar = self.tabs.tabBar()
        idx = bar.tabAt(pos)
        if idx < 0:
            idx = self.tabs.currentIndex()
        if idx < 0:
            return
        view = self.tabs.widget(idx)

        menu = QMenu(self)
        act_new = menu.addAction(tr("Nieuw tabblad vanaf deze locatie"))
        act_pin = menu.addAction(
            "📌 Losmaken" if view.pinned else "📌 Vastzetten op deze locatie"
        )
        act_rename = menu.addAction(tr("Tabblad hernoemen..."))
        act_move = menu.addAction(
            tr("➡ Tabblad naar ander paneel verplaatsen"))
        menu.addSeparator()
        act_close = menu.addAction(tr("Tabblad sluiten\tCtrl+W"))
        act_close.setEnabled(self.tabs.count() > 1)

        chosen = menu.exec(bar.mapToGlobal(pos))
        if chosen == act_move:
            win = self.window()
            dst = win.right if self.side == "left" else win.left
            dst.adopt_tab(self, idx)
        elif chosen == act_new:
            self.add_tab(view.current_path, index=idx + 1)
        elif chosen == act_pin:
            view.pinned = not view.pinned
            if view.pinned:
                view.pinned_path = view.current_path
            else:
                view.pinned_path = None
                view.custom_title = None
            self._set_title(view)
            self._notify_change()
        elif chosen == act_rename:
            default = view.custom_title or os.path.basename(
                view.current_path.rstrip("/")
            ) or "/"
            name, ok = QInputDialog.getText(
                self, "Tabblad hernoemen", "Naam:", text=default
            )
            if ok and name.strip():
                view.custom_title = name.strip()
                self._set_title(view)
                self._notify_change()
        elif chosen == act_close:
            self.close_tab(idx)

    # ---------- persistence ----------
    def save_state(self):
        state = []
        for i in range(self.tabs.count()):
            w = self.tabs.widget(i)
            state.append(
                {
                    # Vastgepinde tab onthoudt altijd zijn pin-locatie,
                    # ook als je er tijdelijk vandaan genavigeerd bent.
                    "path": (w.pinned_path or w.current_path)
                    if w.pinned else w.current_path,
                    "pinned": w.pinned,
                    "title": w.custom_title,
                }
            )
        state.append({"active": self.tabs.currentIndex()})
        return state

    def restore_state(self, state):
        tabs_state = [s for s in state if "path" in s]
        active = next((s.get("active") for s in state if "active" in s), 0)
        while self.tabs.count():
            w = self.tabs.widget(0)
            if w:
                w.shutdown()
            self.tabs.removeTab(0)
            w.deleteLater()
        for s in tabs_state:
            path = remap_stale_lopus_mount(s["path"])
            self.add_tab(path, pinned=s.get("pinned", False), title=s.get("title"))
        if isinstance(active, int) and 0 <= active < self.tabs.count():
            self.tabs.setCurrentIndex(active)

    # ---------- delegation ----------
    def load_path(self, path):
        self.view().load_path(path)

    def go_up(self):
        self.view().go_up()

    def go_back(self):
        self.view().go_back()

    def go_forward(self):
        self.view().go_forward()

    def new_folder(self):
        self.view().new_folder()

    def rename_selected(self):
        self.view().rename_selected()


TRASH_DIR = os.path.expanduser("~/.local/share/Trash")


def _list_trash():
    """Return trash items as dicts: name, trashed_path, orig_path, date."""
    import urllib.parse

    items = []
    files_dir = os.path.join(TRASH_DIR, "files")
    info_dir = os.path.join(TRASH_DIR, "info")
    if not os.path.isdir(files_dir):
        return items
    for name in sorted(os.listdir(files_dir)):
        entry = {
            "name": name,
            "trashed_path": os.path.join(files_dir, name),
            "orig_path": None,
            "date": "",
        }
        info_file = os.path.join(info_dir, name + ".trashinfo")
        if os.path.isfile(info_file):
            try:
                with open(info_file) as f:
                    for line in f:
                        line = line.strip()
                        if line.startswith("Path="):
                            entry["orig_path"] = urllib.parse.unquote(
                                line[len("Path="):]
                            )
                        elif line.startswith("DeletionDate="):
                            entry["date"] = line[len("DeletionDate="):]
            except OSError:
                pass
        items.append(entry)
    return items


def _restore_trash_item(item):
    """Restore one trash item to its original location. Returns error or None."""
    orig = item["orig_path"]
    if not orig:
        return f"Geen oorspronkelijke locatie bekend voor {item['name']}"
    try:
        os.makedirs(os.path.dirname(orig), exist_ok=True)
        shutil.move(item["trashed_path"], orig)
    except OSError as e:
        return f"{item['name']}: {e}"
    info_file = os.path.join(TRASH_DIR, "info", item["name"] + ".trashinfo")
    try:
        if os.path.exists(info_file):
            os.remove(info_file)
    except OSError:
        pass
    return None


class TrashDialog(QDialog):
    """Shows the trash and allows restoring / deleting items."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("Prullenbak"))
        self.resize(720, 420)

        layout = QVBoxLayout(self)
        self.tree = QTreeWidget()
        self.tree.setColumnCount(3)
        self.tree.setHeaderLabels(["Naam", "Oorspronkelijke locatie", "Verwijderd op"])
        header = self.tree.header()
        header.setSectionResizeMode(0, QHeaderView.ResizeMode.Interactive)
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.Interactive)
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.ResizeToContents)
        self.tree.setSortingEnabled(True)
        header.setSortIndicator(0, Qt.SortOrder.AscendingOrder)
        self.tree.setSelectionMode(QTreeWidget.SelectionMode.ExtendedSelection)
        self.tree.setSelectionBehavior(QTreeWidget.SelectionBehavior.SelectRows)
        self.tree.setRootIsDecorated(False)
        self.tree.setAlternatingRowColors(True)
        self.tree.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.tree.customContextMenuRequested.connect(self._context_menu)
        layout.addWidget(self.tree)

        tip = QLabel(
            "Tip: klik op een kolomkop om te sorteren. "
            "Rechtermuisklik op een item voor herstellen/verwijderen."
        )
        layout.addWidget(tip)

        btns = QHBoxLayout()
        btn_restore = QPushButton(tr("Herstellen"))
        btn_restore.clicked.connect(self.restore_selected)
        btn_delete = QPushButton(tr("Definitief verwijderen"))
        btn_delete.clicked.connect(self.delete_selected)
        btn_empty = QPushButton(tr("Prullenbak legen"))
        btn_empty.clicked.connect(self.empty_trash)
        btn_close = QPushButton(tr("Sluiten"))
        btn_close.clicked.connect(self.accept)
        btns.addWidget(btn_restore)
        btns.addWidget(btn_delete)
        btns.addWidget(btn_empty)
        btns.addStretch(1)
        btns.addWidget(btn_close)
        layout.addLayout(btns)

        self.reload()

    def reload(self):
        self.tree.clear()
        for it in _list_trash():
            node = QTreeWidgetItem(
                [it["name"], it["orig_path"] or "?", it["date"]]
            )
            node.setData(0, Qt.ItemDataRole.UserRole, it)
            self.tree.addTopLevelItem(node)

    def _selected_items(self):
        return [
            i.data(0, Qt.ItemDataRole.UserRole)
            for i in self.tree.selectedItems()
            if i.data(0, Qt.ItemDataRole.UserRole)
        ]

    def _context_menu(self, pos):
        item = self.tree.itemAt(pos)
        if item is not None:
            self.tree.setCurrentItem(item)
        menu = QMenu(self)
        act_restore = menu.addAction(tr("Herstellen naar oorspronkelijke locatie"))
        act_delete = menu.addAction(tr("Definitief verwijderen"))
        chosen = menu.exec(self.tree.viewport().mapToGlobal(pos))
        if chosen == act_restore:
            self.restore_selected()
        elif chosen == act_delete:
            self.delete_selected()

    def restore_selected(self):
        items = self._selected_items()
        if not items:
            return
        errors = [e for e in (_restore_trash_item(it) for it in items) if e]
        if errors:
            QMessageBox.warning(self, "Prullenbak", "\n".join(errors))
        self.reload()

    def delete_selected(self):
        items = self._selected_items()
        if not items:
            return
        r = QMessageBox.question(
            self,
            "Prullenbak",
            f"Deze {len(items)} item(s) definitief verwijderen?",
        )
        if r != QMessageBox.StandardButton.Yes:
            return
        for it in items:
            p = it["trashed_path"]
            try:
                if os.path.isdir(p) and not os.path.islink(p):
                    shutil.rmtree(p)
                else:
                    os.remove(p)
                info = os.path.join(TRASH_DIR, "info", it["name"] + ".trashinfo")
                if os.path.exists(info):
                    os.remove(info)
            except OSError as e:
                QMessageBox.warning(self, "Prullenbak", str(e))
        self.reload()

    def empty_trash(self):
        r = QMessageBox.question(
            self, "Prullenbak", "De hele prullenbak legen?"
        )
        if r != QMessageBox.StandardButton.Yes:
            return
        for it in _list_trash():
            p = it["trashed_path"]
            try:
                if os.path.isdir(p) and not os.path.islink(p):
                    shutil.rmtree(p)
                else:
                    os.remove(p)
                info = os.path.join(TRASH_DIR, "info", it["name"] + ".trashinfo")
                if os.path.exists(info):
                    os.remove(info)
            except OSError as e:
                QMessageBox.warning(self, "Prullenbak", str(e))
        self.reload()


# ---------- interne schijven & partities ----------
def list_disks():
    """Lokale schijven/partities via lsblk (JSON), zonder root.

    Geeft lijst van dicts: device, label, fstype, size,
    mountpoint (of None), removable. Swap en lege devices worden
    overgeslagen."""
    try:
        r = subprocess.run(
            ["lsblk", "-J", "-o",
             "NAME,PATH,LABEL,PARTLABEL,FSTYPE,MOUNTPOINTS,SIZE,TYPE,RM"],
            capture_output=True, text=True, timeout=6)
        data = json.loads(r.stdout)
    except Exception:  # noqa: BLE001 - geen lsblk = gewoon geen schijven
        return []
    disks = []

    def walk(nodes):
        for n in nodes or []:
            if n.get("type") in ("part", "crypt"):
                fstype = (n.get("fstype") or "").lower()
                if fstype == "swap":
                    continue
                mps = n.get("mountpoints")
                mp = None
                if isinstance(mps, list):
                    mp = next((m for m in mps if m), None)
                elif mps:
                    mp = mps
                disks.append({
                    "device": n.get("path"),
                    "label": n.get("label") or n.get("partlabel") or "",
                    "fstype": (n.get("fstype") or "?").upper(),
                    "size": n.get("size") or "",
                    "mountpoint": mp,
                    "removable": str(n.get("rm")) == "1",
                })
            walk(n.get("children"))

    walk(data.get("blockdevices"))
    return disks


def disk_display_name(dev):
    """Vriendelijke naam: eigen naam > label > apparaatpad."""
    names = load_ui_settings().get("disk_names", {})
    if dev in names:
        return names[dev]
    for d in list_disks():
        if d["device"] == dev:
            return d["label"] or f"{tr('Schijf')} {dev} ({d['fstype']})"
    return dev or "Schijf"


def udisks_mount(device):
    """Mount via udisks2 (geen root). Geeft (mountpoint, foutmelding)."""
    try:
        r = subprocess.run(["udisksctl", "mount", "-b", device],
                           capture_output=True, text=True, timeout=30)
    except Exception as e:  # noqa: BLE001
        return None, str(e)
    if r.returncode == 0:
        m = re.search(r"at (.+)$", (r.stdout or "").strip())
        return (m.group(1).rstrip(".").strip() if m else None), None
    return None, ((r.stderr or r.stdout or "").strip()
                  or "Onbekende fout bij aankoppelen")


def udisks_unmount(device):
    """Unmount via udisks2. Geeft foutmelding of None."""
    try:
        r = subprocess.run(["udisksctl", "unmount", "-b", device],
                           capture_output=True, text=True, timeout=30)
    except Exception as e:  # noqa: BLE001
        return str(e)
    if r.returncode == 0:
        return None
    return ((r.stderr or r.stdout or "").strip()
            or "Onbekende fout bij ontkoppelen")


def disk_uuid(device):
    """UUID van een partitie opzoeken (voor fstab/systemd-unit)."""
    try:
        r = subprocess.run(["lsblk", "-no", "UUID", device],
                           capture_output=True, text=True, timeout=5)
        uuid = (r.stdout or "").strip().splitlines()[0].strip() \
            if (r.stdout or "").strip() else ""
        return uuid or None
    except Exception:  # noqa: BLE001
        return None


EFI_PARTTYPE = "c12a7328-f81f-11d2-ba4b-00a0c93ec93b"


def disk_is_efi(device):
    """True als dit een EFI-systeempartitie is (die beheert het OS zelf)."""
    try:
        r = subprocess.run(["lsblk", "-no", "PARTTYPE", device],
                           capture_output=True, text=True, timeout=5)
        return EFI_PARTTYPE in (r.stdout or "").lower()
    except Exception:  # noqa: BLE001
        return False


def install_automount(device, fstype, name, parent=None):
    """Zet een schijf vast in /etc/fstab met x-systemd.automount
    (nofail): na reboot staat hij er gewoon, en als hij uit staat
    blokkeert hij de boot niet. Vraagt om beheerderswachtwoord via pkexec.
    Geeft foutmelding of None."""
    uuid = disk_uuid(device)
    if not uuid:
        return ("Deze partitie heeft geen UUID; automatisch aankoppelen "
                "kan hier niet veilig.")
    mount_dir = f"/mnt/lopus/{re.sub(r'[^A-Za-z0-9_.-]', '_', name)}"
    line = (f"UUID={uuid} {mount_dir} {fstype.lower()} "
            f"defaults,nofail,x-systemd.automount,x-systemd.idle-timeout=60"
            f" 0 0")
    script = (
        f"set -e; cp /etc/fstab /etc/fstab.lopus-backup 2>/dev/null || true; "
        f"mkdir -p '{mount_dir}'; "
        f"grep -qF 'UUID={uuid}' /etc/fstab || echo '{line}' >> /etc/fstab"
    )
    try:
        r = subprocess.run(
            ["pkexec", "bash", "-c", script],
            capture_output=True, text=True, timeout=120)
    except Exception as e:  # noqa: BLE001
        return str(e)
    if r.returncode != 0:
        return ((r.stderr or "").strip()
                or "Geen toestemming (geannuleerd?)")
    return None


def _disk_system_managed(d):
    """True als de schijf door het systeem zelf wordt beheerd
    (aangekoppeld op een systeemlocatie als /, /boot, /home e.d.)
    en dus NIET via Lopus ingesteld moet worden."""
    mp = d.get("mountpoint")
    if not mp:
        return False
    for prefix in ("/mnt/", "/media/", "/run/media/", "/home/", "~/"):
        if mp.startswith(prefix):
            return False
    return True


def _disk_already_in_fstab(device):
    """Staat de UUID van deze partitie al in /etc/fstab?"""
    uuid = disk_uuid(device)
    if not uuid:
        return False
    try:
        with open("/etc/fstab") as f:
            return uuid in f.read()
    except OSError:
        return False


# ---------- netwerkshares (SMB via GVfs) ----------
def net_share_gvfs_path(host, share):
    """Zoek het GVfs-koppelpunt van een smb-share (of None)."""
    base = f"/run/user/{os.getuid()}/gvfs"
    if not os.path.isdir(base):
        return None
    h, s = host.lower(), share.lower()
    for e in os.listdir(base):
        if not e.lower().startswith("smb-share:"):
            continue
        parts = e.split(":", 1)[1].split(",")
        kv = {}
        for p in parts:
            if "=" in p:
                k, v = p.split("=", 1)
                kv[k.strip().lower()] = v.strip().lower()
        if kv.get("server") == h and kv.get("share") == s:
            return os.path.join(base, e)
    return None


def net_share_mounted(host, share):
    return net_share_gvfs_path(host, share) is not None


def net_share_connect(host, share):
    """Verbind met smb://host/share via gio (GVfs toont zelf het
    inlogvenster indien nodig). Geeft (gvfs-pad, foutmelding)."""
    try:
        r = subprocess.run(["gio", "mount", f"smb://{host}/{share}"],
                           capture_output=True, text=True, timeout=120)
    except FileNotFoundError:
        return None, "gio (GVfs) is niet geïnstalleerd op dit systeem."
    except Exception as e:  # noqa: BLE001
        return None, str(e)
    if r.returncode != 0:
        return None, ((r.stderr or r.stdout or "").strip()
                      or "Verbinding mislukt.")
    path = net_share_gvfs_path(host, share)
    if not path:
        return None, ("Verbonden, maar het koppelpunt is niet gevonden. "
                      "Is gvfs-smb geïnstalleerd?")
    return path, None


def net_share_disconnect(gvfs_path):
    try:
        r = subprocess.run(["gio", "mount", "-u", gvfs_path],
                           capture_output=True, text=True, timeout=60)
        return None if r.returncode == 0 else (
            (r.stderr or r.stdout or "").strip() or "Ontkoppelen mislukt.")
    except Exception as e:  # noqa: BLE001
        return str(e)


def nfs_export_mountpoint(host, export):
    """Zoek het kernel-mountpoint van een nfs-export (of None)."""
    dev = f"{host}:{export}"
    try:
        with open("/proc/mounts") as f:
            for line in f:
                parts = line.split()
                if len(parts) >= 3 and parts[2].startswith("nfs") \
                        and parts[0] == dev:
                    return parts[1].replace("\\040", " ")
    except OSError:
        pass
    return None


def nfs_mount(host, export):
    """Koppel een nfs-export aan onder /mnt/lopus-nfs (vraagt via pkexec
    om beheerderswachtwoord). Geeft (mountpoint, foutmelding)."""
    if "'" in host or "'" in export or "\\" in host or "\\" in export:
        return None, "Ongeldige tekens in host of export-pad."
    name = re.sub(r"[^A-Za-z0-9_.-]", "_", f"{host}_{export.strip('/')}")
    mp = f"/mnt/lopus-nfs/{name}"
    script = f"mkdir -p '{mp}' && mount -t nfs '{host}:{export}' '{mp}'"
    try:
        r = subprocess.run(["pkexec", "bash", "-c", script],
                           capture_output=True, text=True, timeout=120)
    except Exception as e:  # noqa: BLE001
        return None, str(e)
    if r.returncode != 0:
        return None, ((r.stderr or r.stdout or "").strip()
                      or "Aankoppelen mislukt (geannuleerd?).\n"
                         "Is nfs-utils geïnstalleerd?")
    if not os.path.isdir(mp):
        return None, "Aangekoppeld, maar het mountpoint is niet gevonden."
    return mp, None


def nfs_unmount(mp):
    try:
        r = subprocess.run(["pkexec", "umount", mp],
                           capture_output=True, text=True, timeout=60)
        return None if r.returncode == 0 else (
            (r.stderr or r.stdout or "").strip() or "Ontkoppelen mislukt.")
    except Exception as e:  # noqa: BLE001
        return str(e)


_HOSTNAME_CACHE = {}  # ip -> "ip - pcnaam"


def host_display(host):
    """Leesbare host-kop: '192.168.0.100 - servernaam' als het kan,
    anders gewoon het adres. Resultaat wordt gecached (DNS kan traag zijn)."""
    if not host:
        return "Overig"
    if not re.match(r"^\d+\.\d+\.\d+\.\d+$", host):
        return host
    if host in _HOSTNAME_CACHE:
        return _HOSTNAME_CACHE[host]
    name = None
    try:
        r = subprocess.run(["getent", "hosts", host],
                           capture_output=True, text=True, timeout=2)
        if r.returncode == 0 and r.stdout.strip():
            name = r.stdout.split()[1]
    except Exception:  # noqa: BLE001
        name = None
    disp = f"{host} - {name}" if name and name != host else host
    _HOSTNAME_CACHE[host] = disp
    return disp


def install_netshare_automount(s):
    """Zet een NFS-share vast in /etc/fstab met x-systemd.automount
    (nofail): na reboot staat hij er gewoon, en als de server uit staat
    blokkeert hij de boot niet. Vraagt om beheerderswachtwoord via pkexec.
    Geeft foutmelding of None."""
    host, share = s.get("host", ""), s.get("share", "").rstrip("/")
    if "'" in host or "'" in share or "\\" in host or "\\" in share:
        return "Ongeldige tekens in host of export-pad."
    spec = f"{host}:{share}"
    name = re.sub(r"[^A-Za-z0-9_.-]", "_",
                  s.get("name") or share.strip("/") or "share")
    mp = f"/mnt/lopus-net/{name}"
    line = (f"{spec} {mp} nfs "
            f"defaults,nofail,x-systemd.automount,x-systemd.idle-timeout=60"
            f" 0 0")
    script = (
        f"set -e; cp /etc/fstab /etc/fstab.lopus-backup 2>/dev/null || true; "
        f"mkdir -p '{mp}'; "
        f"grep -qF '{spec} ' /etc/fstab || echo '{line}' >> /etc/fstab; "
        f"systemctl daemon-reload; "
        f"mount '{mp}' 2>/dev/null || true")
    try:
        r = subprocess.run(
            ["pkexec", "bash", "-c", script],
            capture_output=True, text=True, timeout=120)
    except Exception as e:  # noqa: BLE001
        return str(e)
    if r.returncode != 0:
        return ((r.stderr or "").strip()
                or "Geen toestemming (geannuleerd?)")
    return None


def remap_stale_lopus_mount(path):
    """Oude Lopus-mountpaden (/mnt/lopus-nfs/... of /mnt/lopus-net/...) die
    niet meer bestaan, terugmappen naar de huidige koppeling van dezelfde
    share (bijv. een fstab-automount op /home/wiedaar/Server)."""
    if not path or not path.startswith(("/mnt/lopus-nfs/", "/mnt/lopus-net/")):
        return path
    if os.path.isdir(path):
        return path
    norm = path.replace("/", "_")
    for s in load_ui_settings().get("net_shares", []):
        host = (s.get("host") or "").replace("/", "_")
        exp = (s.get("share") or "").strip("/").replace("/", "_")
        if host and exp and norm.endswith(f"{host}_{exp}"):
            mp = net_share_mountpoint(s)
            if mp and os.path.isdir(mp):
                return mp
            break
    return path


def net_share_kind(s):
    """Type share: 'smb' (standaard, oudere instellingen) of 'nfs'."""
    return s.get("type") or "smb"


def net_share_mountpoint(s):
    """Huidig koppelpunt van een opgeslagen share (of None)."""
    if net_share_kind(s) == "nfs":
        return nfs_export_mountpoint(s.get("host", ""),
                                     s.get("share", "").rstrip("/"))
    return net_share_gvfs_path(s.get("host", ""), s.get("share", ""))


def net_share_connect_any(s):
    """Verbind met een share (elk type). Geeft (pad, foutmelding)."""
    host, share = s.get("host", ""), s.get("share", "")
    if net_share_kind(s) == "nfs":
        # Eerst proberen via gvfs (geen root nodig), daarna kernel-mount.
        try:
            r = subprocess.run(["gio", "mount", f"nfs://{host}{share}"],
                               capture_output=True, text=True, timeout=60)
            if r.returncode == 0:
                base = f"/run/user/{os.getuid()}/gvfs"
                try:
                    for e in os.listdir(base):
                        if (e.lower().startswith("nfs-mount")
                                and host.lower() in e.lower()):
                            return os.path.join(base, e), None
                except OSError:
                    pass
        except FileNotFoundError:
            pass
        except Exception:  # noqa: BLE001
            pass
        return nfs_mount(host, share)
    return net_share_connect(host, share)


def net_share_disconnect_any(s):
    """Verbreek de verbinding van een share (elk type)."""
    if net_share_kind(s) == "nfs":
        mp = nfs_export_mountpoint(s.get("host", ""),
                                   s.get("share", "").rstrip("/"))
        if not mp:
            return "Niet aangekoppeld."
        return nfs_unmount(mp)
    return net_share_disconnect(
        net_share_gvfs_path(s.get("host", ""), s.get("share", "")))


def discovered_net_mounts():
    """Netwerk-shares die al door het systeem aangekoppeld zijn
    (fstab/automount/gvfs/rclone) maar nog NIET in de lijst staan.
    Geeft lijst van dicts: {'kind': 'netmount', 'path', 'label', 'host'}."""
    saved = set()
    for s in load_ui_settings().get("net_shares", []):
        mp = net_share_mountpoint(s)
        if mp:
            saved.add(mp.rstrip("/"))
    out = []
    seen = set()
    # Kernel-/FUSE-mounts (nfs, cifs, sshfs, rclone, ...)
    for mp, fstype, dev in remote_mount_details():
        key = mp.rstrip("/")
        if key in saved or key in seen or not os.path.isdir(key):
            continue
        seen.add(key)
        label = os.path.basename(key) or key
        out.append({"kind": "netmount", "path": key, "label": label,
                    "host": mount_host(mp, fstype, dev)})
    return out


# ---------- rclone cloud drives ----------

_RCLONE_SERVICE_TEMPLATE = """[Unit]
Description=Rclone mount voor %I
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
ExecStart=/usr/bin/rclone mount %I: %h/%I --vfs-cache-mode writes \
--vfs-cache-max-age 24h --dir-cache-time 5m --allow-other
ExecStop=/usr/bin/fusermount3 -u %h/%I
Restart=on-failure
RestartSec=10

[Install]
WantedBy=default.target
"""


def rclone_available():
    return bool(shutil.which("rclone"))


def rclone_list_remotes():
    """Namen van geconfigureerde rclone-remotes (zonder dubbele punt)."""
    try:
        r = subprocess.run(["rclone", "listremotes"], capture_output=True,
                           text=True, timeout=15)
    except Exception:  # noqa: BLE001
        return []
    return [ln.strip().rstrip(":") for ln in (r.stdout or "").splitlines()
            if ln.strip()]


def rclone_mountpoint_for(remote):
    """Koppelpunt van een gemounte rclone-remote (of None)."""
    for mp, fstype, dev in remote_mount_details():
        d = dev.strip()
        if d.startswith(remote + ":"):
            return mp
    # gevallen waarin de dev-naam afwijkt: koppelnaam == remotenaam
    for mp, fstype, dev in remote_mount_details():
        if "rclone" in fstype.lower() and os.path.basename(mp) == remote:
            return mp
    return None


def systemd_user_ok():
    """True als een werkende user-systemd aanwezig is."""
    if not shutil.which("systemctl"):
        return False
    try:
        r = subprocess.run(["systemctl", "--user", "show-environment"],
                           capture_output=True, timeout=8)
        return r.returncode == 0
    except Exception:  # noqa: BLE001
        return False


def cloud_systemd_install(remote):
    """Rclone-mount-service instellen en starten (systemd-user).
    Geeft fouttekst of None."""
    svc_dir = os.path.expanduser("~/.config/systemd/user")
    tmpl = os.path.join(svc_dir, "rclone-mount@.service")
    try:
        os.makedirs(svc_dir, exist_ok=True)
        if not os.path.exists(tmpl):
            with open(tmpl, "w") as f:
                f.write(_RCLONE_SERVICE_TEMPLATE)
        os.makedirs(os.path.expanduser(f"~/{remote}"), exist_ok=True)
        subprocess.run(["systemctl", "--user", "daemon-reload"],
                       capture_output=True, timeout=15)
        r = subprocess.run(
            ["systemctl", "--user", "enable", "--now",
             f"rclone-mount@{remote}.service"],
            capture_output=True, text=True, timeout=20)
        if r.returncode != 0:
            return (r.stderr or r.stdout or "systemctl-fout").strip()
    except Exception as e:  # noqa: BLE001
        return str(e)
    return None


def cloud_systemd_stop(remote, disable=False):
    """Mount-service stoppen (en eventueel uitschakelen)."""
    cmds = [["systemctl", "--user", "stop", f"rclone-mount@{remote}.service"]]
    if disable:
        cmds.insert(0, ["systemctl", "--user", "disable",
                        f"rclone-mount@{remote}.service"])
    errs = []
    try:
        subprocess.run(["fusermount3", "-u",
                        os.path.expanduser(f"~/{remote}")],
                       capture_output=True, timeout=15)
    except Exception:  # noqa: BLE001
        pass
    for c in cmds:
        try:
            r = subprocess.run(c, capture_output=True, text=True, timeout=20)
            if r.returncode != 0:
                errs.append((r.stderr or "").strip())
        except Exception as e:  # noqa: BLE001
            errs.append(str(e))
    return "; ".join(e for e in errs if e) or None


def cloud_autostart_desktop(remote):
    """Fallback zonder systemd: autostart-.desktop schrijven + nu mounten.
    Geeft fouttekst of None."""
    adir = os.path.expanduser("~/.config/autostart")
    mp = os.path.expanduser(f"~/{remote}")
    fpath = os.path.join(adir, f"lopus-rclone-{remote}.desktop")
    opts = ("--vfs-cache-mode writes --vfs-cache-max-age 24h "
            "--dir-cache-time 5m --allow-other")
    inhoud = (
        "[Desktop Entry]\nType=Application\n"
        f"Name=Rclone mount {remote}\n"
        f"Exec=rclone mount {remote}: {mp} {opts}\n"
        "X-GNOME-Autostart-enabled=true\nNoDisplay=true\n")
    try:
        os.makedirs(mp, exist_ok=True)
        os.makedirs(adir, exist_ok=True)
        with open(fpath, "w") as f:
            f.write(inhoud)
        subprocess.Popen(["rclone", "mount", f"{remote}:", mp]
                         + opts.split())
    except Exception as e:  # noqa: BLE001
        return str(e)
    return None


def cloud_autostart_remove(remote):
    fpath = os.path.expanduser(f"~/.config/autostart/"
                               f"lopus-rclone-{remote}.desktop")
    try:
        if os.path.exists(fpath):
            os.remove(fpath)
    except OSError:
        pass


def open_in_terminal(cmd):
    """Open cmd in de standaard-terminalapp van de gebruiker."""
    for term, prefix in (
            ("x-terminal-emulator", ["x-terminal-emulator", "-e"]),
            ("konsole", ["konsole", "-e"]),
            ("gnome-terminal", ["gnome-terminal", "--"]),
            ("xfce4-terminal", ["xfce4-terminal", "-x"]),
            ("alacritty", ["alacritty", "-e"]),
            ("kitty", ["kitty", ]),
            ("xterm", ["xterm", "-e"])):
        if shutil.which(term):
            try:
                subprocess.Popen(prefix + [cmd])
                return True
            except OSError:
                continue
    return False



    """NFS-exports op een host opzoeken via showmount.
    Geeft lijst van export-paden (of None bij fout)."""
    try:
        r = subprocess.run(["showmount", "-e", host],
                           capture_output=True, text=True, timeout=20)
    except FileNotFoundError:
        return None
    except Exception:  # noqa: BLE001
        return None
    if r.returncode != 0:
        return None
    out = []
    for line in (r.stdout or "").splitlines()[1:]:  # kopregel overslaan
        parts = line.split()
        if parts and parts[0].startswith("/"):
            out.append(parts[0])
    return out


def smb_list_shares(host):
    """Shares op een host opzoeken via smbclient (gast-toegang).
    Geeft lijst van sharenamen (of None bij fout)."""
    try:
        r = subprocess.run(
            ["smbclient", "-L", f"//{host}", "-N", "-g"],
            capture_output=True, text=True, timeout=20)
    except FileNotFoundError:
        return None
    shares = []
    for line in (r.stdout or "").splitlines():
        # smbclient -g: "Disk|share|comment"
        parts = line.split("|")
        if len(parts) >= 2 and parts[0].strip() == "Disk":
            name = parts[1].strip()
            if name and not name.endswith("$"):
                shares.append(name)
    return shares if r.returncode == 0 else None


class DirectoryTree(QTreeWidget):
    """Vertical directory tree shown as sidebar, incl. local disks."""

    open_path = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setColumnCount(1)
        self.setHeaderHidden(True)
        self.itemExpanded.connect(self._on_expanded)
        self.itemClicked.connect(self._on_clicked)
        self.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.customContextMenuRequested.connect(self._context_menu)
        self.reload()

    def reload(self):
        # Na een herbouw even muur-events negeren: nagesleepte klik- of
        # contextmenu-events mogen niet op de NIEUWE items uitkomen.
        self._suppress_until = time.monotonic() + 1.2
        self.clear()
        for label, path in (("Computer", "/"), ("Home", os.path.expanduser("~"))):
            it = self._make_item(label, path)
            it.setText(0, label)
            self.addTopLevelItem(it)
        # 💾 Schijven-groep: interne schijven/partities (Windows D:/E:-gevoel)
        disks = list_disks()
        if disks:
            grp = QTreeWidgetItem([tr("💾 Schijven")])
            f = grp.font(0)
            f.setBold(True)
            grp.setFont(0, f)
            grp.setFlags(Qt.ItemFlag.ItemIsEnabled)
            self.addTopLevelItem(grp)
            for d in disks:
                # EFI-/boot-partities verbergen: die beheert het OS zelf
                if disk_is_efi(d["device"]):
                    continue
                name = disk_display_name(d["device"])
                status = "" if d["mountpoint"] else tr("  —  niet aangekoppeld")
                it = QTreeWidgetItem([f"💾 {name}{status}"])
                it.setData(0, Qt.ItemDataRole.UserRole,
                           {"kind": "disk", **d})
                if not d["mountpoint"]:
                    it.setForeground(0, QColor("#8a8a8a"))
                tip = f"{d['device']}  •  {d['fstype']}  •  {d['size']}"
                tip += (f"\nAangekoppeld op: {d['mountpoint']}"
                        if d["mountpoint"]
                        else "\nKlik om aan te koppelen en te openen")
                it.setToolTip(0, tip)
                grp.addChild(it)
            grp.setExpanded(True)
        # ☁ Cloud-groep: rclone-remotes (OneDrive e.d.), elk met status;
        # ➕ item om via de GUI een nieuwe cloud-drive toe te voegen.
        cgrp = QTreeWidgetItem(["☁ Cloud"])
        cf = cgrp.font(0)
        cf.setBold(True)
        cgrp.setFont(0, cf)
        cgrp.setFlags(Qt.ItemFlag.ItemIsEnabled)
        self.addTopLevelItem(cgrp)
        # ➕ altijd tonen — ook zonder rclone (klik geeft dan
        # installatie-instructies in plaats van stil niets doen)
        addc = QTreeWidgetItem([tr("➕ Cloud drive toevoegen...")])
        addc.setData(0, Qt.ItemDataRole.UserRole, {"kind": "cloud_add"})
        addc.setForeground(0, QColor("#6aa0c9"))
        addc.setToolTip(0,
                        "Nieuwe cloud-drive koppelen (OneDrive, Gdrive, "
                        "...)")
        cgrp.addChild(addc)
        if rclone_available():
            for name in rclone_list_remotes():
                mp = rclone_mountpoint_for(name)
                status = "" if mp else tr("  —  niet gemount")
                cit = QTreeWidgetItem([f"☁ {name}{status}"])
                cit.setData(0, Qt.ItemDataRole.UserRole,
                            {"kind": "cloud", "name": name, "mount": mp})
                if not mp:
                    cit.setForeground(0, QColor("#8a8a8a"))
                    cit.setToolTip(0, f"{name}:  — klik om te mounten "
                                      "en te openen")
                else:
                    cit.setToolTip(0, f"{name}:\nAangekoppeld op {mp}")
                cgrp.addChild(cit)
            cgrp.setExpanded(True)
        else:
            hint = QTreeWidgetItem(["(rclone niet geïnstalleerd)"])
            hint.setFlags(Qt.ItemFlag.ItemIsEnabled)
            hint.setForeground(0, QColor("#8a8a8a"))
            cgrp.addChild(hint)

        # 🌐 Netwerk-groep: opgeslagen netwerkshares (SMB én NFS) plus
        # netwerk-mounts die het systeem al aangekoppeld heeft,
        # geordend per host (computernaam/IP).
        shares = load_ui_settings().get("net_shares", [])
        ngrp = QTreeWidgetItem([tr("🌐 Netwerk")])
        nf = ngrp.font(0)
        nf.setBold(True)
        ngrp.setFont(0, nf)
        ngrp.setFlags(Qt.ItemFlag.ItemIsEnabled)
        self.addTopLevelItem(ngrp)
        if not shares:
            actadd = QTreeWidgetItem([tr("➕ Netwerkshare toevoegen...")])
            actadd.setData(0, Qt.ItemDataRole.UserRole,
                           {"kind": "net_add"})
            actadd.setForeground(0, QColor("#6aa0c9"))
            ngrp.addChild(actadd)

        def host_group(host):
            """Subkop per host (met kind-items, uitgeklapt)."""
            label = host_display(host)
            grp = QTreeWidgetItem([f"🖥 {label}"])
            gf = grp.font(0)
            gf.setBold(True)
            grp.setFont(0, gf)
            grp.setFlags(Qt.ItemFlag.ItemIsEnabled)
            grp.setToolTip(0, f"Shares van {label}")
            ngrp.addChild(grp)
            grp.setExpanded(True)
            return grp

        groups = {}

        def group_for(host):
            key = (host or "Overig").lower()
            if key not in groups:
                groups[key] = host_group(host)
            return groups[key]

        for s in shares:
            is_nfs = net_share_kind(s) == "nfs"
            mp = net_share_mountpoint(s)
            status = "" if mp else tr("  —  niet verbonden")
            it = QTreeWidgetItem(
                [f"🔗 {s.get('name') or s.get('share', '?')}{status}"])
            it.setData(0, Qt.ItemDataRole.UserRole,
                       {"kind": "netshare", **s})
            if not mp:
                it.setForeground(0, QColor("#8a8a8a"))
            url = (f"nfs://{s.get('host')}{s.get('share')}" if is_nfs
                   else f"smb://{s.get('host')}/{s.get('share')}")
            it.setToolTip(0, url + (f"\nAangekoppeld op: {mp}" if mp else ""))
            group_for(s.get("host", "")).addChild(it)
        # Automatisch gevonden: al aangekoppelde netwerk-shares die niet
        # in de lijst staan (bijv. via fstab/automount gekoppelde NFS-shares).
        for m in discovered_net_mounts():
            # rclone-cloud-drives staan in hun eigen ☁ Cloud-groep
            if m.get("host") == "rclone":
                continue
            it = QTreeWidgetItem([f"🌐 {m['label']}"])
            it.setData(0, Qt.ItemDataRole.UserRole, m)
            tip = (f"{m['path']}\nAutomatisch gevonden netwerk-share"
                   + (f"\nHost: {m['host']}" if m.get("host") else ""))
            it.setToolTip(0, tip)
            group_for(m.get("host")).addChild(it)
        ngrp.setExpanded(True)

    def _selected_disk(self, item):
        d = item.data(0, Qt.ItemDataRole.UserRole) if item else None
        return d if isinstance(d, dict) and d.get("kind") == "disk" else None

    def _open_disk(self, disk):
        """Schijf openen; zo nodig eerst aankoppelen."""
        if disk["mountpoint"]:
            self.open_path.emit(disk["mountpoint"])
            return
        win = self.window()
        mp, err = udisks_mount(disk["device"])
        if err or not mp:
            QMessageBox.warning(
                self, APP_NAME,
                f"Kan {disk['device']} niet aankoppelen:\n{err}")
            return
        disk["mountpoint"] = mp
        self.reload()
        self.open_path.emit(mp)
        if isinstance(win, MainWindow):
            win.set_status(f"💾 {disk_display_name(disk['device'])} "
                           "aangekoppeld", 3000)

    def _open_netshare(self, s):
        """Netwerkshare openen; zo nodig eerst verbinden."""
        path = net_share_mountpoint(s)
        if not path:
            path, err = net_share_connect_any(s)
            if err or not path:
                kind = net_share_kind(s)
                url = (f"nfs://{s.get('host')}{s.get('share')}" if kind == "nfs"
                       else f"smb://{s.get('host')}/{s.get('share')}")
                QMessageBox.warning(
                    self, APP_NAME,
                    f"Kan {url} niet verbinden:\n{err}")
                return
            self.reload()
        if path:
            self.open_path.emit(path)
            # Open tabbladen die nog naar het oude mountpad wijzen nu
            # meenemen naar de nieuwe koppeling.
            win = self.window()
            if isinstance(win, MainWindow):
                win.remap_stale_tabs()

    def _open_cloud(self, d):
        """Cloud-drive openen; nog niet gemount? Dan eerst aankoppelen."""
        name = d["name"]
        mp = rclone_mountpoint_for(name)
        if not mp:
            win = self.window()
            if systemd_user_ok():
                err = cloud_systemd_install(name)
            else:
                err = cloud_autostart_desktop(name)
            if err:
                QMessageBox.warning(self, APP_NAME,
                                    f"Kan {name} niet aankoppelen:\n{err}")
                return
            mp = rclone_mountpoint_for(name)
            self.reload()
            if isinstance(win, MainWindow):
                win.set_status(f"☁ {name} aangekoppeld", 3000)
        if mp:
            self.open_path.emit(mp)
            win = self.window()
            if isinstance(win, MainWindow):
                win.remap_stale_tabs()

    def _add_cloud_dialog(self):
        """Nieuwe cloud-drive toevoegen: rclone config in een terminal,
        daarna remote kiezen en koppelen (systemd of autostart-fallback)."""
        import traceback
        log_error("TRACE cloud-dialog geopend vanaf:\n"
                  + "".join(traceback.format_stack()[-8:-1]))
        if not rclone_available():
            QMessageBox.warning(
                self, APP_NAME,
                "rclone is nog niet geïnstalleerd op deze computer.\n\n"
                "Installeer het eerst, bijvoorbeeld:\n"
                "  sudo apt install rclone\n"
                "of via jouw pakketbeheerder / softwarecentrum.\n\n"
                "Daarna dit venster opnieuw openen.")
            return
        dlg = QDialog(self)
        dlg.setWindowTitle(tr("Cloud drive toevoegen"))
        dlg.resize(620, 380)
        if not self._begin_add(dlg):
            return  # er staat al een toevoeg-dialoog open
        lay = QVBoxLayout(dlg)
        stappen = QLabel(
            "1️⃣  Open rclone config in een terminal en doorloop de vragen\n"
            "     (provider kiezen, inloggen in je browser, naam geven).\n"
            "2️⃣  Kom hier terug en druk op 🔄 Opnieuw opzoeken —\n"
            "     de nieuwe remote verschijnt dan in de lijst.\n"
            "3️⃣  Kies de remote en druk op 🔌 Koppelen.")
        stappen.setWordWrap(True)
        lay.addWidget(stappen)

        row0 = QHBoxLayout()
        btn_cfg = QPushButton(tr("⚙ 1. rclone config openen..."))
        btn_cfg.clicked.connect(lambda: (
            None if open_in_terminal("rclone config")
            else QMessageBox.warning(
                self, APP_NAME, "Geen terminal-programma gevonden.")))
        row0.addWidget(btn_cfg)
        row0.addStretch(1)
        lay.addLayout(row0)

        h = QHBoxLayout()
        h.addWidget(QLabel("Remote:"))
        combo = QComboBox()
        combo.setMinimumWidth(260)
        h.addWidget(combo, 1)
        btn_scan = QPushButton(tr("🔄 2. Opnieuw opzoeken"))

        def vul_combos():
            huidig = [combo.itemData(i) for i in range(combo.count())]
            nieuw = rclone_list_remotes()
            combo.clear()
            for n in nieuw:
                combo.addItem(n, n)
            return nieuw
        # eerste vulling (vreemd te noemen: gelijk tonen wat er al is)
        vul_combos()
        btn_scan.clicked.connect(vul_combos)
        h.addWidget(btn_scan)
        lay.addLayout(h)

        r2 = QHBoxLayout()
        r2.addWidget(QLabel(tr("Koppelpunt:")))
        na_edit = QLineEdit()
        r2.addWidget(na_edit, 1)
        combo.currentIndexChanged.connect(
            lambda: na_edit.setText(combo.currentData() or ""))
        combo.currentIndexChanged.emit(0)
        lay.addLayout(r2)

        self.lbl_method = QLabel("")
        self.lbl_method.setStyleSheet("color: gray;")
        self.lbl_method.setText(
            "Koppelmethode: systemd-user (aanbevolen)"
            if systemd_user_ok() else
            "Koppelmethode: autostart-bestand (geen systemd gevonden)")
        lay.addWidget(self.lbl_method)

        brow = QHBoxLayout()
        btn_go = QPushButton(tr("🔌 Koppelen"))
        btn_cancel = QPushButton(tr("Annuleren"))
        btn_cancel.clicked.connect(dlg.reject)
        brow.addStretch(1)
        brow.addWidget(btn_go)
        brow.addWidget(btn_cancel)
        lay.addLayout(brow)

        def ga():
            naam = (combo.currentData() or "").rstrip(":").strip()
            punt = na_edit.text().strip().replace("/", "") or naam
            if not naam:
                QMessageBox.warning(self, APP_NAME,
                                    "Kies eerst een remote.")
                return
            al_gemount = rclone_mountpoint_for(naam)
            if al_gemount:
                # staat er al: niets opnieuw installeren/mounten
                mp = al_gemount
            else:
                if systemd_user_ok():
                    # koppelpunt volgt de remotenaam (service-sjabloon
                    # gebruikt %I)
                    err = cloud_systemd_install(punt or naam)
                else:
                    err = cloud_autostart_desktop(punt or naam)
                if err:
                    QMessageBox.warning(dlg, APP_NAME,
                                        f"Koppelen mislukt:\n{err}")
                    return
                mp = os.path.expanduser(f"~/{punt or naam}")
            dlg.accept()
            self.reload()
            self.open_path.emit(mp)
            QMessageBox.information(
                self, APP_NAME,
                f"☁ {naam} is gekoppeld op {mp}.\n"
                + ("Hij staat ingesteld om bij elke opstart automatisch "
                   "te mounten (systemd)." if systemd_user_ok() else
                   "Een autostart-bestand zorgt dat hij bij het inloggen "
                   "weer gemount wordt."))
        btn_go.clicked.connect(ga)
        dlg.exec()

    def _add_netshare_dialog(self):
        """Dialoogje: netwerkshare toevoegen (host + share + naam)."""
        dlg = QDialog(self)
        dlg.setWindowTitle(tr("Netwerkshare toevoegen"))
        if not self._begin_add(dlg):
            return  # er staat al een toevoeg-dialoog open
        lay = QVBoxLayout(dlg)
        form = QVBoxLayout()
        r0 = QHBoxLayout()
        r0.addWidget(QLabel("Type share:"))
        type_combo = QComboBox()
        type_combo.addItem("Samba / Windows (SMB)", "smb")
        type_combo.addItem("Linux / NFS-export", "nfs")
        r0.addWidget(type_combo, 1)
        form.addLayout(r0)
        r1 = QHBoxLayout()
        r1.addWidget(QLabel(tr("Computer (IP-adres of naam):")))
        host_edit = QLineEdit()
        host_edit.setPlaceholderText(tr("bijv. 192.168.0.100 of server"))
        r1.addWidget(host_edit, 1)
        btn_browse = QPushButton(tr("Shares opzoeken..."))
        r1.addWidget(btn_browse)
        form.addLayout(r1)
        r2 = QHBoxLayout()
        self._share_label = QLabel("Share:")
        r2.addWidget(self._share_label)
        share_combo = QComboBox()
        share_combo.setEditable(True)
        share_combo.setEnabled(False)
        r2.addWidget(share_combo, 1)
        form.addLayout(r2)

        def on_type_changed():
            is_nfs = type_combo.currentData() == "nfs"
            self._share_label.setText(
                "Export-pad (bijv. /srv/data):" if is_nfs else "Share:")
            btn_browse.setText(
                "Exports opzoeken..." if is_nfs else "Shares opzoeken...")
            share_combo.setEnabled(is_nfs or share_combo.count() > 0)

        type_combo.currentIndexChanged.connect(on_type_changed)
        r3 = QHBoxLayout()
        r3.addWidget(QLabel(tr("Naam in de zijbalk:")))
        name_edit = QLineEdit()
        name_edit.setPlaceholderText(tr("leeg = sharenaam gebruiken"))
        r3.addWidget(name_edit, 1)
        form.addLayout(r3)
        lay.addLayout(form)

        def browse():
            host = host_edit.text().strip()
            if not host:
                QMessageBox.information(
                    dlg, APP_NAME, "Vul eerst een IP-adres of naam in.")
                return
            is_nfs = type_combo.currentData() == "nfs"
            btn_browse.setText(tr("⏳ Zoeken..."))
            QApplication.processEvents()
            if is_nfs:
                found = nfs_list_exports(host)
                btn_browse.setText(tr("Exports opzoeken..."))
                if found is None:
                    QMessageBox.warning(
                        dlg, APP_NAME,
                        "Kon de exports niet opzoeken.\n"
                        "Is showmount (nfs-utils) geïnstalleerd en staat "
                        "de NFS-server aan? Je kunt het export-pad ook "
                        "handmatig intypen (bijv. /srv/data).")
                    share_combo.setEnabled(True)
                    return
                if not found:
                    QMessageBox.information(
                        dlg, APP_NAME,
                        "Geen exports gevonden op deze computer\n"
                        "(of de server geeft geen lijst vrij — typ het "
                        "export-pad dan handmatig in).")
                share_combo.clear()
                share_combo.addItems(found)
                share_combo.setEnabled(True)
                if found and not name_edit.text().strip():
                    name_edit.setText(host)
                return
            btn_browse.setText(tr("Shares opzoeken..."))
            found = smb_list_shares(host)
            btn_browse.setText(tr("Shares opzoeken..."))
            if found is None:
                QMessageBox.warning(
                    dlg, APP_NAME,
                    "Kon de shares niet opzoeken.\n"
                    "Is smbclient geïnstalleerd? Je kunt de sharenaam "
                    "ook handmatig intypen.")
                share_combo.setEnabled(True)
                return
            if not found:
                QMessageBox.information(
                    dlg, APP_NAME,
                    "Geen shares gevonden op deze computer\n"
                    "(of er is een wachtwoord nodig — typ de sharenaam "
                    "dan handmatig in).")
            share_combo.clear()
            share_combo.addItems(found)
            share_combo.setEnabled(True)
            if found and not name_edit.text().strip():
                name_edit.setText(host)

        btn_browse.clicked.connect(browse)
        btns = QHBoxLayout()
        ok = QPushButton("Toevoegen")
        ok.setDefault(True)
        cancel = QPushButton(tr("Annuleren"))
        cancel.clicked.connect(dlg.reject)

        def accept_add():
            host = host_edit.text().strip()
            share = share_combo.currentText().strip()
            name = name_edit.text().strip() or share
            stype = type_combo.currentData() or "smb"
            if not host or not share:
                QMessageBox.information(
                    dlg, APP_NAME,
                    "Vul minimaal een computer en een share in.")
                return
            data = load_ui_settings()
            lst = data.setdefault("net_shares", [])
            lst.append({"name": name, "host": host, "share": share,
                        "type": stype})
            win = self.window()
            if isinstance(win, MainWindow):
                win._write_settings(data)
            self.reload()
            dlg.accept()

        ok.clicked.connect(accept_add)
        btns.addStretch(1)
        btns.addWidget(cancel)
        btns.addWidget(ok)
        lay.addLayout(btns)
        dlg.exec()

    @staticmethod
    def _subdirs(path):
        out = []
        try:
            with os.scandir(path) as it:
                for e in it:
                    if e.name.startswith("."):
                        continue
                    try:
                        if e.is_dir(follow_symlinks=False):
                            out.append(e.name)
                    except OSError:
                        continue
        except OSError:
            return []
        return sorted(out, key=str.lower)

    @staticmethod
    def _make_item(name, full):
        child = QTreeWidgetItem([name])
        child.setData(0, Qt.ItemDataRole.UserRole, full)
        child.setIcon(0, ICON_PROVIDER.icon(QFileInfo(full)))
        # Placeholder child so the expand arrow shows without stat'ing now
        dummy = QTreeWidgetItem(["⏳"])
        dummy.setData(0, Qt.ItemDataRole.UserRole + 1, "dummy")
        child.addChild(dummy)
        return child

    def _add_guard(self, kind):
        """Voorkom dat één muisgebaar (of een nagesleepte event)
        meerdere / verkeerde toevoeg-dialoogen opent."""
        now = time.monotonic()
        if getattr(self, "_last_add_kind", None) == kind and \
                now - getattr(self, "_last_add_t", 0.0) < 0.6:
            return False
        # zelfs een ANDER soort dialoog binnen een halve seconde: negeer,
        # dat is bijna zeker een nagesleept tweede event van hetzelfde
        # gebaar (bijv. klik + contextmenu op hetzelfde item).
        if now - getattr(self, "_last_add_t", 0.0) < 0.35:
            return False
        self._last_add_kind = kind
        self._last_add_t = now
        return True

    def _begin_add(self, dlg):
        """Nieuwe toevoeg-dialoog registreren. Er mag er maar ÉÉN tegelijk
        open staan (gestapeld annuleren/vensters is anders mogelijk)."""
        prev = getattr(self, "_active_add_dlg", None)
        if prev is not None and prev.isVisible():
            prev.raise_()
            prev.activateWindow()
            return False
        self._active_add_dlg = dlg

        def _clear(*_a):
            if getattr(self, "_active_add_dlg", None) is dlg:
                self._active_add_dlg = None
        dlg.finished.connect(_clear)
        return True

    def _context_menu(self, pos):
        if time.monotonic() < getattr(self, "_suppress_until", 0.0):
            return  # vers herbouwde boom: oude events negeren
        item = self.itemAt(pos)
        d = item.data(0, Qt.ItemDataRole.UserRole) if item else None
        if isinstance(d, dict) and d.get("kind") == "cloud_add":
            if self._add_guard("cloud"):
                self._add_cloud_dialog()
            return
        if isinstance(d, dict) and d.get("kind") == "cloud":
            menu = QMenu(self)
            act_open = menu.addAction(tr("📂 Openen"))
            mp = rclone_mountpoint_for(d["name"])
            if mp:
                act_restart = menu.addAction(tr("🔄 Herstarten (mount verversen)"))
                act_stop = menu.addAction(tr("⏏ Ontkoppelen"))
                menu.addSeparator()
                act_disable = menu.addAction(
                    tr("🚫 Uitschakelen bij opstart + ontkoppelen"))
            else:
                act_restart = act_stop = act_disable = None
                act_start = menu.addAction(tr("🔌 Aankoppelen"))
            chosen = menu.exec(self.viewport().mapToGlobal(pos))
            if chosen is None:
                return
            if chosen == act_open:
                self._open_cloud(d)
            elif chosen == act_start:
                self._open_cloud(d)
            elif chosen == act_restart and mp:
                if systemd_user_ok():
                    cloud_systemd_stop(d["name"])
                    err = cloud_systemd_install(d["name"])
                else:
                    err = cloud_autostart_desktop(d["name"])
                if err:
                    QMessageBox.warning(self, APP_NAME,
                                        f"Herstart mislukt:\n{err}")
                self.reload()
                self.open_path.emit(mp)
            elif chosen == act_stop and mp:
                if systemd_user_ok():
                    err = cloud_systemd_stop(d["name"])
                else:
                    try:
                        subprocess.run(["fusermount3", "-u", mp],
                                       capture_output=True, timeout=15)
                        err = None
                    except OSError as e2:
                        err = str(e2)
                if err:
                    QMessageBox.warning(self, APP_NAME,
                                        f"Ontkoppelen mislukt:\n{err}")
                self.reload()
            elif chosen == act_disable:
                disable = cloud_systemd_stop(d["name"], disable=True)
                cloud_autostart_remove(d["name"])
                if disable:
                    QMessageBox.warning(self, APP_NAME,
                                        f"Uitschakelen meldde:\n{disable}")
                self.reload()
            return
        # Netwerk-share aangeklikt
        if isinstance(d, dict) and d.get("kind") == "net_add":
            if self._add_guard("net"):
                self._add_netshare_dialog()
            return
        # Automatisch gevonden (al aangekoppelde) netwerk-share
        if isinstance(d, dict) and d.get("kind") == "netmount":
            menu = QMenu(self)
            act_open = menu.addAction(tr("📂 Openen"))
            act_copy = menu.addAction(tr("📋 Koppelpunt kopiëren"))
            chosen = menu.exec(self.viewport().mapToGlobal(pos))
            if chosen == act_open:
                self.open_path.emit(d["path"])
            elif chosen == act_copy:
                QApplication.clipboard().setText(d["path"])
            return
        if isinstance(d, dict) and d.get("kind") == "netshare":
            menu = QMenu(self)
            act_open = menu.addAction(tr("📂 Openen"))
            mp = net_share_mountpoint(d)
            act_conn = menu.addAction(
                "🔌 Verbinden" if not mp else "⏏ Verbreken")
            menu.addSeparator()
            act_rename = menu.addAction(tr("✏ Naam geven..."))
            act_auto = None
            if net_share_kind(d) == "nfs":
                act_auto = menu.addAction(
                    tr("⚙ Automatisch aankoppelen bij opstart..."))
            act_remove = menu.addAction(tr("🗑 Uit de lijst verwijderen"))
            chosen = menu.exec(self.viewport().mapToGlobal(pos))
            if chosen is None:
                return
            if chosen == act_open:
                self._open_netshare(d)
            elif chosen == act_conn:
                if mp:
                    err = net_share_disconnect_any(d)
                    if err:
                        QMessageBox.warning(self, APP_NAME,
                                            f"Verbreken mislukt:\n{err}")
                else:
                    self._open_netshare(d)
                self.reload()
            elif chosen == act_rename:
                name, ok = QInputDialog.getText(
                    self, "Naam geven",
                    "Hoe moet deze share heten in de zijbalk?",
                    text=d.get("name", ""))
                if ok and name.strip():
                    data = load_ui_settings()
                    for s in data.setdefault("net_shares", []):
                        if (s.get("host") == d.get("host")
                                and s.get("share") == d.get("share")):
                            s["name"] = name.strip()
                    win = self.window()
                    if isinstance(win, MainWindow):
                        win._write_settings(data)
                    self.reload()
            elif chosen == act_auto:
                err = install_netshare_automount(d)
                if err:
                    QMessageBox.warning(
                        self, APP_NAME,
                        f"Automatisch aankoppelen instellen mislukt:\n{err}")
                else:
                    QMessageBox.information(
                        self, APP_NAME,
                        "Deze share staat nu in /etc/fstab met automount.\n"
                        "Na een herstart wordt hij vanzelf aangekoppeld "
                        "zodra je hem opent (of direct als de server "
                        "bereikbaar is). Als de server uit staat, wordt "
                        "de boot niet geblokkeerd.")
                self.reload()
            elif chosen == act_remove:
                r = QMessageBox.question(
                    self, APP_NAME,
                    "Deze share uit de lijst verwijderen?\n"
                    "(De share zelf wordt niet verwijderd, alleen uit "
                    "de zijbalk.)")
                if r != QMessageBox.StandardButton.Yes:
                    return
                try:
                    net_share_disconnect_any(d)
                except Exception:  # noqa: BLE001
                    pass
                data = load_ui_settings()
                data["net_shares"] = [
                    s for s in data.get("net_shares", [])
                    if not (s.get("host") == d.get("host")
                            and s.get("share") == d.get("share")
                            and net_share_kind(s) == net_share_kind(d))]
                win = self.window()
                if isinstance(win, MainWindow):
                    win._write_settings(data)
                self.reload()
            return
        # Op een groepskop? Dan eventueel share toevoegen aanbieden.
        if item is not None and (item.text(0).count("Netwerk") or item.text(0).count("Network")):
            menu = QMenu(self)
            act_add = menu.addAction(tr("➕ Netwerkshare toevoegen..."))
            chosen = menu.exec(self.viewport().mapToGlobal(pos))
            if chosen == act_add:
                if self._add_guard("net"):
                    self._add_netshare_dialog()
            return
        disk = self._selected_disk(item)
        if not disk:
            return
        menu = QMenu(self)
        act_mount = menu.addAction(
            "🔌 Aankoppelen" if not disk["mountpoint"]
            else "⏏ Ontkoppelen")
        menu.addSeparator()
        act_rename = menu.addAction(tr("✏ Naam geven..."))
        act_auto = menu.addAction(
            tr("⚙ Automatisch aankoppelen bij opstart..."))
        act_copy = menu.addAction(tr("📋 Apparaatpad kopiëren"))
        chosen = menu.exec(self.viewport().mapToGlobal(pos))
        if chosen is None:
            return
        if chosen == act_mount:
            if disk["mountpoint"]:
                err = udisks_unmount(disk["device"])
                if err:
                    QMessageBox.warning(self, APP_NAME,
                                        f"Ontkoppelen mislukt:\n{err}")
                self.reload()
            else:
                self._open_disk(disk)
        elif chosen == act_rename:
            cur = load_ui_settings().get("disk_names", {}).get(
                disk["device"], "")
            name, ok = QInputDialog.getText(
                self, "Naam geven",
                f"Hoe moet {disk['device']} heten in de zijbalk?\n"
                "(leeg = standaardnaam gebruiken)",
                text=cur or disk.get("label", ""))
            if ok:
                data = load_ui_settings()
                names = data.setdefault("disk_names", {})
                if name.strip():
                    names[disk["device"]] = name.strip()
                else:
                    names.pop(disk["device"], None)
                win = self.window()
                if isinstance(win, MainWindow):
                    win._write_settings(data)
                self.reload()
        elif chosen == act_auto:
            # Check 1: wordt de schijf al door het systeem beheerd?
            if _disk_system_managed(disk):
                QMessageBox.information(
                    self, APP_NAME,
                    "Deze schijf wordt al door het systeem beheerd.\n\n"
                    f"Ze is nu aangekoppeld op: {disk['mountpoint']}\n"
                    "(een systeemlocatie zoals /, /boot of /home)\n\n"
                    "Er hoeft en mag hier niets ingesteld worden — "
                    "CachyOS koppelt deze zelf aan. Je kunt er gewoon "
                    "een eigen naam aan geven via ✏ Naam geven.")
                return
            # Check 2: staat hij al in fstab?
            if _disk_already_in_fstab(disk["device"]):
                QMessageBox.information(
                    self, APP_NAME,
                    "Deze schijf staat al in /etc/fstab — hij wordt dus "
                    "al automatisch aangekoppeld door je systeem.\n"
                    "Er hoeft niets meer ingesteld worden.")
                return
            # Check 3: EFI-/boot-partitie? Daar moet Lopus niet aan komen
            if disk_is_efi(disk["device"]):
                QMessageBox.information(
                    self, APP_NAME,
                    "Dit is een EFI-/boot-partitie (het kleine VFAT-\n"
                    "partitietje). Die beheert je besturingssysteem zelf:\n"
                    "hij wordt alleen tijdelijk aangekoppeld wanneer het "
                    "systeem hem nodig heeft (bijv. bij kernel-updates).\n\n"
                    "Niets instellen dus — alles is al in orde.")
                return
            r = QMessageBox.question(
                self, APP_NAME,
                f"Schijf {disk['device']} automatisch aankoppelen bij het "
                "opstarten?\n\nDe schijf wordt dan bij elke herstart "
                "vanzelf aangekoppeld en zichtbaar.\n"
                "Er wordt nu om je beheerderswachtwoord gevraagd.")
            if r != QMessageBox.StandardButton.Yes:
                return
            err = install_automount(
                disk["device"], disk["fstype"],
                disk_display_name(disk["device"]), self)
            if err:
                QMessageBox.warning(self, APP_NAME, f"Lukt niet:\n{err}")
            else:
                QMessageBox.information(
                    self, APP_NAME,
                    "Automatisch aankoppelen ingesteld.\n"
                    "Na de volgende herstart staat de schijf er vanzelf.")
        elif chosen == act_copy:
            QApplication.clipboard().setText(disk["device"] or "")

    def _on_expanded(self, item):
        path = item.data(0, Qt.ItemDataRole.UserRole)
        if not path:
            return
        # Fill lazily: only when the placeholder dummy is still there
        if item.childCount() == 1 and item.child(0).text(0) == "⏳":
            item.takeChildren()
            for name in self._subdirs(path):
                item.addChild(self._make_item(name, os.path.join(path, name)))
        else:
            # Refresh children of already-loaded nodes cheaply
            pass

    def _on_clicked(self, item, _col):
        if time.monotonic() < getattr(self, "_suppress_until", 0.0):
            return  # vers herbouwde boom: oude events negeren
        d = item.data(0, Qt.ItemDataRole.UserRole)
        if isinstance(d, dict) and d.get("kind") == "disk":
            self._open_disk(d)
            return
        if isinstance(d, dict) and d.get("kind") == "net_add":
            if self._add_guard("net"):
                self._add_netshare_dialog()
            return
        if isinstance(d, dict) and d.get("kind") == "cloud_add":
            if self._add_guard("cloud"):
                self._add_cloud_dialog()
            return
        if isinstance(d, dict) and d.get("kind") == "cloud":
            self._open_cloud(d)
            return
        if isinstance(d, dict) and d.get("kind") == "netmount":
            self.open_path.emit(d["path"])
            return
        if isinstance(d, dict) and d.get("kind") == "netshare":
            self._open_netshare(d)
            return
        if d and d != "/":
            self.open_path.emit(d)


class AppearanceDialog(QDialog):
    """Font / size / colour settings, incl. saving/loading named themes."""

    def __init__(self, theme, main=None):
        super().__init__(main)
        self.main = main
        self.setWindowTitle(tr("Weergave-instellingen"))
        self.resize(520, 420)
        self.theme = dict(theme)
        self._color_labels = {}

        layout = QVBoxLayout(self)
        form_rows = []

        # Font family + size
        row = QHBoxLayout()
        row.addWidget(QLabel(tr("Lettertype:")))
        self.font_combo = QFontComboBox()
        if self.theme.get("font_family"):
            self.font_combo.setCurrentText(self.theme["font_family"])
        row.addWidget(self.font_combo, 1)
        row.addWidget(QLabel(tr("Grootte:")))
        self.size_spin = QSpinBox()
        self.size_spin.setRange(0, 32)
        self.size_spin.setValue(int(self.theme.get("font_size") or 0))
        self.size_spin.setToolTip("0 = systeemstandaard")
        row.addWidget(self.size_spin)
        form_rows.append(row)

        def color_row(label, key):
            r = QHBoxLayout()
            r.addWidget(QLabel(label))
            btn = QPushButton(tr("Kies kleur..."))
            cur = QLabel(self.theme.get(key) or "(standaard)")
            btn.clicked.connect(lambda: self._pick_color(key, cur))
            r.addWidget(cur, 1)
            r.addWidget(btn)
            form_rows.append(r)
            self._color_labels[key] = cur
            setattr(self, f"_btn_{key}", btn)

        color_row("Tekstkleur (buiten vensters):", "color_text")
        color_row("Achtergrond (buiten vensters):", "color_bg_window")
        color_row("Tekstkleur bestandslijsten:", "color_text_pane")
        color_row("Achtergrond bestandslijsten:", "color_bg_pane")
        color_row("Tekstkleur navigatiebalk:", "color_nav_text")
        color_row("Achtergrond navigatiebalk:", "color_bg_nav")
        color_row("Achtergrond actieve tabblad:", "color_tab_selected")
        color_row("Tekstkleur actieve tabblad:", "color_tab_selected_text")
        color_row("Menu achtergrond:", "color_menu_bg")
        color_row("Menu tekstkleur:", "color_menu_text")
        color_row("Menu gemarkeerd (hover):", "color_menu_sel")
        color_row("Menu scheidslijnen (streepjes):", "color_menu_sep")

        # Bestandskleuren in een eigen popup (hoort bij de functie
        # "Kleurcodering van bestanden" via de 🖼-knop)
        row_files = QHBoxLayout()
        row_files.addWidget(QLabel(
            tr("Bestandskleuren (per soort, voor kleurcodering):")))
        btn_files = QPushButton(tr("Wijzigen..."))
        btn_files.setToolTip(
            "Kleuren per bestandssoort voor \"Kleurcodering van "
            "bestanden\" (🖼-knop)")
        btn_files.clicked.connect(self._open_file_colors)
        row_files.addWidget(btn_files)
        form_rows.append(row_files)

        # Bar heights
        layout.addWidget(QLabel("Balkhoogtes (0 = automatisch):"))
        self._size_spins = {}
        for key, label in (
            ("size_functiebalk", "Functiebalk:"),
            ("size_knoppenbalk", "Knoppenbalk:"),
            ("size_tabbladstrook", "Tabbladstrook:"),
            ("size_navigatiebalk", "Navigatiebalk:"),
        ):
            r = QHBoxLayout()
            r.addWidget(QLabel(label))
            spin = QSpinBox()
            spin.setRange(0, 120)
            spin.setSuffix(" px")
            spin.setValue(int(self.theme.get(key) or 0))
            r.addWidget(spin)
            r.addStretch(1)
            layout.addLayout(r)
            self._size_spins[key] = spin

        for r in form_rows:
            layout.addLayout(r)
        layout.addStretch(1)

        reset = QPushButton(tr("Alles terug naar standaard"))
        reset.clicked.connect(self._reset)
        layout.addWidget(reset)

        # Thema opslaan / laden binnen het weergavevenster
        theme_row = QHBoxLayout()
        btn_theme_save = QPushButton(tr("Thema opslaan als..."))
        btn_theme_save.clicked.connect(self._save_theme)
        btn_theme_load = QPushButton(tr("Thema laden..."))
        btn_theme_load.clicked.connect(self._load_theme)
        theme_row.addWidget(btn_theme_save)
        theme_row.addWidget(btn_theme_load)
        theme_row.addStretch(1)
        layout.addLayout(theme_row)

        btns = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok
            | QDialogButtonBox.StandardButton.Cancel
        )
        btns.accepted.connect(self.accept)
        btns.rejected.connect(self.reject)
        lay_buttons = QHBoxLayout()
        lay_buttons.addStretch(1)
        lay_buttons.addWidget(btns)
        layout.addLayout(lay_buttons)

    def _pick_color(self, key, label):
        col = QColorDialog.getColor(
            QColor(self.theme[key]) if self.theme.get(key) else QColor(),
            self, "Kies kleur",
        )
        if col.isValid():
            self.theme[key] = col.name()
            label.setText(col.name())

    def _open_file_colors(self):
        """Popup met alleen de bestandskleuren (kleurcodering per soort)."""
        from PyQt6.QtWidgets import QDialog, QVBoxLayout, QHBoxLayout
        dlg = QDialog(self)
        dlg.setWindowTitle("Bestandskleuren")
        lay = QVBoxLayout(dlg)
        info = QLabel(
            "Kleuren van bestandsnamen per soort, gebruikt door "
            "\"Kleurcodering van bestanden\"\n(🖼-knop in de padbalk).\n"
            "Leeg = ingebouwde standaardkleur.")
        lay.addWidget(info)
        labels = {}
        for label, key in (
            ("Afbeeldingen:", "color_file_afbeelding"),
            ("Video:", "color_file_video"),
            ("Audio:", "color_file_audio"),
            ("Archieven:", "color_file_archief"),
            ("Documenten:", "color_file_document"),
        ):
            r = QHBoxLayout()
            r.addWidget(QLabel(label))
            cur = QLabel(self.theme.get(key) or "(standaard)")
            btn = QPushButton(tr("Kies kleur..."))
            btn.clicked.connect(
                lambda _c=False, k=key, l=cur: self._pick_color(k, l))
            r.addWidget(cur, 1)
            r.addWidget(btn)
            lay.addLayout(r)
            labels[key] = cur
        # Houd ze bij zodat Reset/Thema-laden ze ook bijwerken
        self._color_labels.update(labels)
        close = QPushButton(tr("Sluiten"))
        close.clicked.connect(dlg.accept)
        h = QHBoxLayout()
        h.addStretch(1)
        h.addWidget(close)
        lay.addLayout(h)
        dlg.exec()

    def _reset(self):
        self.theme = dict(DEFAULT_THEME)
        self.font_combo.setCurrentIndex(-1)
        self.size_spin.setValue(0)
        for key, lbl in self._color_labels.items():
            lbl.setText(tr("(standaard)"))
        QMessageBox.information(self, "Weergave", "Standaard hersteld.")

    def _save_theme(self):
        if self.main is None:
            return
        from PyQt6.QtWidgets import QInputDialog

        name, ok = QInputDialog.getText(
            self, "Thema opslaan", "Naam van het thema (bijv. Kerst):"
        )
        if not (ok and name.strip()):
            return
        t = self.result_theme()
        if self.main is not None:
            # Vensterlayout (geometrie, splitters, balken) meenemen
            t["window_layout"] = self.main.capture_layout()
        data = load_ui_settings()
        themes = data.setdefault("themes", {})
        themes[name.strip()] = {k: v for k, v in t.items() if v}
        self.main._write_settings(data)
        QMessageBox.information(
            self, "Thema", f"Thema '{name.strip()}' opgeslagen."
        )

    def _load_theme(self):
        themes = load_ui_settings().get("themes", {})
        if not themes:
            QMessageBox.information(
                self, "Thema laden", "Er zijn nog geen thema's opgeslagen."
            )
            return
        names = sorted(themes.keys())
        name, ok = QInputDialog.getItem(
            self, "Thema laden", "Kies een thema:", names, 0, False
        )
        if not ok:
            return
        loaded = dict(DEFAULT_THEME)
        loaded.update(
            {k: v for k, v in themes[name].items()
             if k in DEFAULT_THEME or k == "window_layout"}
        )
        self.theme = loaded
        # Update widgets to reflect the loaded theme
        self.font_combo.setCurrentText(loaded.get("font_family") or "")
        self.size_spin.setValue(int(loaded.get("font_size") or 0))
        for key, lbl in self._color_labels.items():
            lbl.setText(loaded.get(key) or "(standaard)")

    def result_theme(self):
        t = dict(DEFAULT_THEME)
        fam = self.font_combo.currentText().strip()
        if fam:
            t["font_family"] = fam
        if self.size_spin.value():
            t["font_size"] = self.size_spin.value()
        for k in (
            "color_text",
            "color_bg_window",
            "color_text_pane",
            "color_bg_pane",
            "color_nav_text",
            "color_bg_nav",
            "color_tab_selected",
            "color_tab_selected_text",
            "color_menu_bg",
            "color_menu_text",
            "color_menu_sel",
            "color_menu_sep",
            "color_file_afbeelding",
            "color_file_video",
            "color_file_audio",
            "color_file_archief",
            "color_file_document",
        ):
            if self.theme.get(k):
                t[k] = self.theme[k]
        for k, spin in self._size_spins.items():
            t[k] = spin.value()  # always persist (0 = automatic)
        if isinstance(self.theme.get("window_layout"), dict):
            t["window_layout"] = self.theme["window_layout"]
        return t


class PermissionsDialog(QDialog):
    """Uitgebreide rechten-dialoog (zoals Double Commander)."""

    def __init__(self, path, parent=None):
        super().__init__(parent)
        self.path = path
        self.st = os.lstat(path)
        self.is_dir = statmod.S_ISDIR(self.st.st_mode)
        self.setWindowTitle(f"Rechten — {os.path.basename(path)}")
        self.resize(420, 460)

        lay = QVBoxLayout(self)
        lay.addWidget(QLabel(f"Bestandsnaam: {os.path.basename(path)}"))

        # 3x3 checkbox grid
        grid = QGridLayout()
        subjects = [("Eigenaar", 6), ("Groep", 3), ("Ander", 0)]
        rights = ["Lezen", "Schrijven", "Uitvoeren"]
        for c, rname in enumerate(rights):
            grid.addWidget(QLabel(rname), 0, c + 1)
        self._checks = {}
        for r, (sname, shift) in enumerate(subjects, start=1):
            grid.addWidget(QLabel(sname), r, 0)
            for c, bit in enumerate((4, 2, 1)):
                cb = QCheckBox()
                cb.setChecked(bool(self.st.st_mode & (bit << shift)))
                grid.addWidget(cb, r, c + 1)
                self._checks[(shift, bit)] = cb
        lay.addLayout(grid)

        # Special bits
        bits_row = QHBoxLayout()
        bits_row.addWidget(QLabel("Bits:"))
        self._special = {}
        for name, bit in (("SUID", 0o4000), ("SGID", 0o2000), ("Klevend", 0o1000)):
            cb = QCheckBox(name)
            cb.setChecked(bool(self.st.st_mode & bit))
            bits_row.addWidget(cb)
            self._special[bit] = cb
        bits_row.addStretch(1)
        lay.addLayout(bits_row)

        # Octaal + tekst
        oct_row = QHBoxLayout()
        oct_row.addWidget(QLabel("Octaal:"))
        self.octal_edit = QLineEdit(format(int(self.st.st_mode) & 0o7777, "o"))
        oct_row.addWidget(self.octal_edit)
        self.mode_label = QLabel(statmod.filemode(self.st.st_mode))
        oct_row.addWidget(self.mode_label)
        lay.addLayout(oct_row)

        def sync_from_checks():
            m = self._mode_from_checks()
            self.octal_edit.setText(format(m, "o"))
            self.mode_label.setText(statmod.filemode(m | (self.st.st_mode & ~0o7777)))

        def sync_from_octal():
            try:
                m = int(self.octal_edit.text().strip() or "0", 8)
            except ValueError:
                return
            self._set_checks(m)
            self.mode_label.setText(statmod.filemode(m | (self.st.st_mode & ~0o7777)))

        for cb in self._checks.values():
            cb.toggled.connect(sync_from_checks)
        for cb in self._special.values():
            cb.toggled.connect(sync_from_checks)
        self.octal_edit.textEdited.connect(sync_from_octal)

        lay.addSpacing(8)

        # Eigenaar / groep
        import grp
        import pwd

        own_row = QHBoxLayout()
        own_row.addWidget(QLabel(tr("Eigenaar:")))
        self.owner_combo = QComboBox()
        try:
            owners = sorted((pw.pw_name for pw in pwd.getpwall()))
        except (OSError, KeyError):
            owners = []
        self.owner_combo.addItems(owners)
        try:
            self.owner_combo.setCurrentText(pwd.getpwuid(self.st.st_uid).pw_name)
        except KeyError:
            pass
        own_row.addWidget(self.owner_combo, 1)
        lay.addLayout(own_row)

        grp_row = QHBoxLayout()
        grp_row.addWidget(QLabel(tr("Groep:")))
        self.group_combo = QComboBox()
        try:
            groups = sorted((gr.gr_name for gr in grp.getgrall()))
        except (OSError, KeyError):
            groups = []
        self.group_combo.addItems(groups)
        try:
            self.group_combo.setCurrentText(grp.getgrgid(self.st.st_gid).gr_name)
        except KeyError:
            pass
        grp_row.addWidget(self.group_combo, 1)
        lay.addLayout(grp_row)

        self.recursive_cb = QCheckBox(tr("Recursief (alle submappen en bestanden)"))
        self.recursive_cb.setVisible(self.is_dir)
        lay.addWidget(self.recursive_cb)

        btns = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok
            | QDialogButtonBox.StandardButton.Cancel
        )
        btns.accepted.connect(self.accept)
        btns.rejected.connect(self.reject)
        lay.addWidget(btns)

    def _mode_from_checks(self):
        m = 0
        for (shift, bit), cb in self._checks.items():
            if cb.isChecked():
                m |= bit << shift
        for bit, cb in self._special.items():
            if cb.isChecked():
                m |= bit
        return m

    def _set_checks(self, mode):
        for (shift, bit), cb in self._checks.items():
            cb.setChecked(bool(mode & (bit << shift)))

    def result_mode(self):
        return int(self.octal_edit.text().strip() or "0", 8)

    def apply(self):
        import pwd

        mode = self.result_mode()
        recursive = self.recursive_cb.isVisible() and self.recursive_cb.isChecked()
        owner_name = self.owner_combo.currentText().strip()
        group_name = self.group_combo.currentText().strip()
        try:
            uid = pwd.getpwnam(owner_name).pw_uid
        except KeyError:
            uid = -1
        try:
            import grp

            gid = grp.getgrnam(group_name).gr_gid
        except (KeyError, ImportError):
            gid = -1

        targets = [self.path]
        if recursive:
            for root, dirs, files in os.walk(self.path):
                targets.extend(os.path.join(root, x) for x in dirs + files)
        errors = []
        for t in targets:
            try:
                os.chmod(t, mode)
                if uid != -1 or gid != -1:
                    os.chown(t, uid if uid != -1 else -1,
                             gid if gid != -1 else -1)
            except OSError as e:
                errors.append(f"{t}: {e}")
        return errors


class DirPickerDialog(QDialog):
    """Map kiezen met de eigen Lopus-mapboom (i.p.v. Dolphin)."""

    def __init__(self, start_dir, parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("Kies een map"))
        self.resize(420, 520)
        self.chosen = None

        lay = QVBoxLayout(self)
        self.tree = DirectoryTree()
        self.tree.itemClicked.connect(self._on_click)
        lay.addWidget(self.tree, 1)
        self.label = QLabel(f"Geselecteerd: {start_dir}")
        lay.addWidget(self.label)
        btns = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok
            | QDialogButtonBox.StandardButton.Cancel
        )
        btns.accepted.connect(self._accept)
        btns.rejected.connect(self.reject)
        lay.addWidget(btns)
        self._current = start_dir

    def _on_click(self, item, _col):
        path = item.data(0, Qt.ItemDataRole.UserRole)
        if path:
            self._current = path
            self.label.setText(f"Geselecteerd: {path}")

    def _accept(self):
        self.chosen = self._current
        self.accept()


class FolderOptionsDialog(QDialog):
    """Kolommen kiezen voor een specifieke map (+ optioneel submappen).

    Linkerkant: boom met hoofdgroepen (Audio/Video/Foto's/Diverse) die
    uitklappen met alle beschikbare tags. Rechterkant: gekozen kolommen.
    """

    def __init__(self, directory, main, parent=None):
        super().__init__(parent)
        self.main = main
        self.main_write = main._write_settings if main else None
        self.directory = directory
        self.setWindowTitle(f"Mapopties — {os.path.basename(directory)}")
        self.resize(620, 520)

        lay = QVBoxLayout(self)
        row = QHBoxLayout()
        row.addWidget(QLabel(tr("Map:")))
        self.folder_edit = QLineEdit(directory)
        self.folder_edit.setReadOnly(True)
        row.addWidget(self.folder_edit, 1)
        pick_btn = QPushButton(tr("Kies..."))
        pick_btn.clicked.connect(self._pick_folder)
        row.addWidget(pick_btn)
        lay.addLayout(row)

        mid = QHBoxLayout()
        left_col = QVBoxLayout()
        left_col.addWidget(QLabel("Beschikbare tags per soort:"))
        self.tree = QTreeWidget()
        self.tree.setHeaderHidden(True)
        self.tree.setSelectionMode(
            QAbstractItemView.SelectionMode.ExtendedSelection)
        self._fill_tree()
        left_col.addWidget(self.tree, 1)
        mid.addLayout(left_col, 1)

        btns = QVBoxLayout()
        add_btn = QPushButton(">")
        add_btn.setFixedWidth(40)
        rem_btn = QPushButton("<")
        rem_btn.setFixedWidth(40)
        up_btn = QPushButton("▲")
        up_btn.setFixedWidth(40)
        down_btn = QPushButton("▼")
        down_btn.setFixedWidth(40)
        for b in (add_btn, rem_btn, up_btn, down_btn):
            btns.addWidget(b)
        btns.addStretch(1)
        mid.addLayout(btns)

        right_col = QVBoxLayout()
        right_col.addWidget(QLabel(
            tr("Gekozen kolommen (volgorde = kolomvolgorde):")))
        self.sel = QListWidget()
        right_col.addWidget(self.sel, 1)
        mid.addLayout(right_col, 1)
        lay.addLayout(mid, 1)

        add_btn.clicked.connect(self._add_tags)
        rem_btn.clicked.connect(self._remove)
        up_btn.clicked.connect(lambda: self._move(-1))
        down_btn.clicked.connect(lambda: self._move(1))

        self.sub_cb = QCheckBox(tr("Ook toepassen op alle submappen"))
        self.sub_cb.setChecked(True)
        lay.addWidget(self.sub_cb)

        apply_btn = QPushButton(tr("Toepassen"))
        apply_btn.clicked.connect(self._apply)
        close_btn = QPushButton(tr("Sluiten"))
        close_btn.clicked.connect(self.close)
        brow = QHBoxLayout()
        brow.addWidget(apply_btn)
        brow.addWidget(close_btn)
        brow.addStretch(1)
        lay.addLayout(brow)

        self._load_selection()

    # ---------- tag tree ----------
    def _fill_tree(self):
        for group, label in TYPE_LABELS:
            top = QTreeWidgetItem([label])
            for key, lbl, groups in COLUMN_LIBRARY:
                if group in groups:
                    child = QTreeWidgetItem([lbl])
                    child.setData(0, Qt.ItemDataRole.UserRole, key)
                    top.addChild(child)
            self.tree.addTopLevelItem(top)
            top.setExpanded(False)

    def _selected_tags(self):
        out = []
        for item in self.tree.selectedItems():
            key = item.data(0, Qt.ItemDataRole.UserRole)
            if key and item.parent() is not None:
                out.append(item)
        return out

    def _add_tags(self):
        chosen = {self.sel.item(i).data(Qt.ItemDataRole.UserRole)
                  for i in range(self.sel.count())}
        for item in self._selected_tags():
            key = item.data(0, Qt.ItemDataRole.UserRole)
            if key not in chosen:
                it = QListWidgetItem(item.text(0))
                it.setData(Qt.ItemDataRole.UserRole, key)
                self.sel.addItem(it)
                chosen.add(key)

    def _remove(self):
        for it in list(self.sel.selectedItems()):
            self.sel.takeItem(self.sel.row(it))

    def _move(self, direction):
        row = self.sel.currentRow()
        if row < 0:
            return
        new = row + direction
        if 0 <= new < self.sel.count():
            it = self.sel.takeItem(row)
            self.sel.insertItem(new, it)
            self.sel.setCurrentRow(new)

    # ---------- folder picker ----------
    def _pick_folder(self):
        dlg = DirPickerDialog(self.folder_edit.text() or "/", self)
        if dlg.exec() and dlg.chosen:
            self.directory = dlg.chosen
            self.folder_edit.setText(dlg.chosen)
            self._load_selection()

    # ---------- load/save ----------
    def _load_selection(self):
        try:
            selected = set(self.main.focused_panel().view()._dir_columns_for(
                self.directory))
        except Exception:
            selected = set()
        self.sel.clear()
        for key, label, _g in COLUMN_LIBRARY:
            if key in selected:
                it = QListWidgetItem(COLUMN_LABELS[key])
                it.setData(Qt.ItemDataRole.UserRole, key)
                self.sel.addItem(it)

    def _apply(self):
        directory = os.path.expanduser(self.folder_edit.text().strip())
        if not os.path.isdir(directory):
            QMessageBox.warning(self, "Mapopties", "Geen geldige map.")
            return
        keys = [self.sel.item(i).data(Qt.ItemDataRole.UserRole)
                for i in range(self.sel.count())]
        data = load_ui_settings()
        mapping = data.setdefault("dir_columns", {})
        if keys:
            mapping[directory] = {"cols": keys,
                                  "sub": self.sub_cb.isChecked()}
        else:
            mapping.pop(directory, None)
        data["dir_columns"] = mapping
        if callable(self.main_write):
            self.main_write(data)
        self.main.left.refresh()
        self.main.right.refresh()
class MainWindow(QMainWindow):










    def __init__(self):
        super().__init__()
        self.setWindowTitle(APP_NAME)
        # Taal ALLEREERST inladen: alles wat daarna wordt opgebouwd
        # (zijbalk, panelen, menu's) gebruikt dan meteen de juiste taal.
        _init_lang = load_ui_settings().get("language")
        if _init_lang in ("nl", "en"):
            set_language(_init_lang)
        self.theme = current_theme()
        apply_theme(self.theme)
        self.resize(1280, 760)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        self.splitter = splitter
        self.left = FilePanel(side="left")
        self.right = FilePanel(side="right")
        splitter.addWidget(self.left)
        splitter.addWidget(self.right)
        splitter.setSizes([640, 640])

        # Sidebar with directory tree
        self.dir_tree = DirectoryTree()
        self.dir_tree.open_path.connect(
            lambda p: self.focused_panel().load_path(p)
        )
        outer = QSplitter(Qt.Orientation.Horizontal)
        self.outer_splitter = outer
        outer.addWidget(self.dir_tree)
        outer.addWidget(splitter)
        outer.setSizes([220, 1060])
        outer.setCollapsible(1, False)
        self.setCentralWidget(outer)

        # Preview dock (F3)
        self.preview = QDockWidget("Voorbeeld", self)
        self.preview.setObjectName("PreviewDock")
        self.preview.setAllowedAreas(
            Qt.DockWidgetArea.RightDockWidgetArea
            | Qt.DockWidgetArea.LeftDockWidgetArea
        )
        from PyQt6.QtWidgets import QStackedWidget as _QSW

        self._pv_stack = _QSW()
        self._pv_placeholder = QLabel(tr("Geen selectie"))
        self._pv_placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._pv_image = QLabel()
        self._pv_image.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._pv_text = QPlainTextEdit()
        self._pv_text.setReadOnly(True)
        # Video-preview (QtMultimedia; optioneel)
        self._pv_player = None
        self._pv_video = None
        try:
            from PyQt6.QtMultimedia import (
                QMediaPlayer, QAudioOutput)
            from PyQt6.QtMultimediaWidgets import QVideoWidget
            self._pv_video = QVideoWidget()
            self._pv_audio_out = QAudioOutput()
            self._pv_player = QMediaPlayer()
            self._pv_player.setAudioOutput(self._pv_audio_out)
            self._pv_player.setVideoOutput(self._pv_video)
            self._pv_video_btn = QPushButton("⏸/▶ Pauzeren/afspelen")
            self._pv_vw = QWidget()
            vwl = QVBoxLayout(self._pv_vw)
            vwl.setContentsMargins(0, 0, 0, 0)
            vwl.addWidget(self._pv_video, 1)
            hb = QHBoxLayout()
            hb.addStretch(1)
            b = self._pv_video_btn
            b.clicked.connect(self._toggle_pv_playback)
            hb.addWidget(b)
            from PyQt6.QtWidgets import QSlider
            s = QSlider(Qt.Orientation.Horizontal)
            s.setFixedWidth(120)
            s.setRange(0, 100)
            s.setValue(int(self._pv_audio_out.volume() * 100))
            s.setToolTip("Volume video-preview")
            s.valueChanged.connect(
                lambda v: self._pv_audio_out.setVolume(v / 100))
            self._pv_vol = s
            hb.addWidget(s)
            hb.addStretch(1)
            vwl.addLayout(hb)
            for wdgt in (self._pv_placeholder, self._pv_image, self._pv_text,
                         self._pv_vw):
                self._pv_stack.addWidget(wdgt)
            self._pv_player.errorOccurred.connect(
                lambda err, es: self.set_status(
                    f"⚠ Video-preview: {es or 'fout'}", 5000))
        except Exception:  # noqa: BLE001  (QtMultimedia ontbreekt)
            for wdgt in (self._pv_placeholder, self._pv_image, self._pv_text):
                self._pv_stack.addWidget(wdgt)
        self.preview.setWidget(self._pv_stack)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, self.preview)
        self.preview.hide()

        # Single shared bottom bar (replaces per-pane footer + QStatusBar)
        container = QWidget()
        vlay = QVBoxLayout(container)
        vlay.setContentsMargins(0, 0, 0, 0)
        vlay.setSpacing(0)
        vlay.addWidget(outer)
        self.bottom_label = QLabel("")
        self.bottom_label.setStyleSheet(
            "padding: 3px 6px; background: palette(mid);"
        )
        self.bottom_label.setWordWrap(False)
        self.bottom_label.setFixedHeight(26)
        self.bottom_label.setSizePolicy(
            QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Fixed
        )
        vlay.addWidget(self.bottom_label)
        self.setCentralWidget(container)

        # Pre-load language from config before building menus & toolbars
        _init_lang = load_ui_settings().get("language")
        if _init_lang in ("nl", "en"):
            set_language(_init_lang)

        self._build_menubar()
        self._build_toolbar()
        self.set_status(tr("Klaar"))
        self.worker = None
        self.clipboard = {"mode": None, "paths": []}

        # Tab persistence: wire change callbacks and restore saved session
        self._active_panel = None       # paneel waar de focus zit
        self._panel_footers = {}        # paneel -> laatste footer-tekst
        for panel in (self.left, self.right):
            panel.change_callback = self.save_settings
            panel.path_changed.connect(lambda _p: self.save_settings())
            panel.selection_callback = (
                lambda pv: self._update_preview_for(pv)
            )
            panel.footer_changed.connect(
                lambda txt, p=panel: self._panel_footer(p, txt))
        QApplication.instance().focusChanged.connect(self._on_focus_changed)
        self._active_panel = self.left
        self.restore_settings()
        # Vensterlayout uit het thema (geometrie, splitters, balken)
        saved_layout = load_ui_settings().get("window_layout")
        if isinstance(saved_layout, dict):
            self.apply_layout(saved_layout)

        # Restore last known footer text after transient messages
        self._status_timer = QTimer(self)
        self._status_timer.setSingleShot(True)
        self._status_timer.timeout.connect(
            lambda: self.bottom_label.setText(getattr(self, "_last_footer", "")))

    def set_status(self, text, timeout=0):
        """Show text in the shared bottom bar; optional auto-restore."""
        self.bottom_label.setText(text)
        if timeout:
            self._status_timer.start(timeout)
        else:
            self._last_footer = text

    def _panel_footer(self, panel, text):
        """Footer van een paneel binnen: alleen tonen als dat paneel actief
        is; anders bewaren tot het paneel weer actief wordt. Zo toont de
        onderbalk nooit informatie van een achtergrondtab/paneel."""
        self._panel_footers[panel] = text
        if panel is self._active_panel:
            self.set_status(text)

    def _on_focus_changed(self, _old, new):
        w = new
        while w is not None:
            if isinstance(w, FilePanel):
                if w in (self.left, self.right) and w is not self._active_panel:
                    self._active_panel = w
                    # Direct de footer van dit paneel (en zijn actieve tab)
                    self.set_status(
                        self._panel_footers.get(w, "") or "")
                return
            w = w.parentWidget()

    # ---------- settings ----------
    # ---------- vensterlayout (deel van een thema) ----------
    def capture_layout(self):
        """Leg de vensterlayout vast: geometrie, splitters, balken."""
        bars = {}
        for key, w in (("functiebalk", getattr(self, "func_bar", None)),
                       ("knoppenbalk", getattr(self, "btn_bar", None)),
                       ("zijbalk", getattr(self, "dir_tree", None))):
            if w is not None:
                bars[key] = bool(w.isVisible())
        return {
            "geometry": bytes(self.saveGeometry().toBase64()).decode(),
            "outer_sizes": list(self.outer_splitter.sizes()),
            "splitter_sizes": list(self.splitter.sizes()),
            "bars": bars,
        }

    def apply_layout(self, layout):
        """Pas een opgeslagen vensterlayout toe."""
        try:
            from PyQt6.QtCore import QByteArray
            geo = layout.get("geometry")
            if geo:
                self.restoreGeometry(
                    QByteArray.fromBase64(geo.encode()))
            outer = layout.get("outer_sizes")
            if outer and len(outer) == 2:
                self.outer_splitter.setSizes([int(outer[0]), int(outer[1])])
            inner = layout.get("splitter_sizes")
            if inner and len(inner) == 2:
                self.splitter.setSizes([int(inner[0]), int(inner[1])])
            bars = layout.get("bars", {})
            for key, act in (("functiebalk", getattr(self, "act_tb", None)),
                             ("knoppenbalk", getattr(self, "act_bar", None)),
                             ("zijbalk", getattr(self, "act_sidebar", None))):
                if key in bars and act is not None:
                    act.setChecked(bool(bars[key]))
        except Exception as e:  # noqa: BLE001
            log_error("vensterlayout toepassen faalde", e)

    def change_language(self, lang):
        """Wissel de actieve taal en sla op."""
        set_language(lang)
        self.save_settings()
        res = QMessageBox.question(
            self, APP_NAME,
            "Taal gewijzigd naar Nederlands.\nHerstart Lopus om de hele interface bij te werken.\n\nNu herstarten?"
            if lang == "nl" else
            "Language changed to English.\nRestart Lopus to update the entire interface.\n\nRestart now?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        if res == QMessageBox.StandardButton.Yes:
            # os.execv vervangt dit proces door een verse Lopus: geen
            # weesproces, geen hangende thread, Konsole wordt netjes
            # vrijgegeven.
            os.execv(sys.executable, [sys.executable] + sys.argv)

    def _check_first_run_language(self):
        """Vraag bij de allereerste start om de taal te kiezen."""
        from PyQt6.QtCore import QLocale
        sys_lang = QLocale.system().name().lower()
        default_lang = "nl" if sys_lang.startswith("nl") else "en"

        dlg = QDialog(self)
        dlg.setWindowTitle(tr("Lopus — Language / Taal"))
        lay = QVBoxLayout(dlg)
        lbl = QLabel(tr("Welkom bij Lopus! Kies je taal:\nWelcome to Lopus! Select your language:"))
        lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lay.addWidget(lbl)

        btn_lay = QHBoxLayout()
        btn_nl = QPushButton("Nederlands 🇳🇱")
        btn_en = QPushButton("English 🇬🇧")

        chosen = [default_lang]

        def select_nl():
            chosen[0] = "nl"
            dlg.accept()

        def select_en():
            chosen[0] = "en"
            dlg.accept()

        btn_nl.clicked.connect(select_nl)
        btn_en.clicked.connect(select_en)
        btn_lay.addWidget(btn_nl)
        btn_lay.addWidget(btn_en)
        lay.addLayout(btn_lay)

        dlg.exec()
        set_language(chosen[0])
        self.save_settings()
        if chosen[0] == "en":
            # Proces volledig vervangen: geen hangend restproces.
            os.execv(sys.executable, [sys.executable] + sys.argv)

    def save_settings(self):
        try:
            os.makedirs(os.path.dirname(CONFIG_PATH), exist_ok=True)
            data = load_ui_settings()
            try:
                from translations import CURRENT_LANG
                data["language"] = CURRENT_LANG
            except ImportError:
                pass
            view_l = self.left.view()
            view_r = self.right.view()
            data["col_widths_left"] = (
                [view_l.tree.columnWidth(c) for c in range(view_l.tree.columnCount())]
                if view_l else None
            )
            data["col_widths_right"] = (
                [view_r.tree.columnWidth(c) for c in range(view_r.tree.columnCount())]
                if view_r else None
            )
            data["functiebalk"] = self.func_bar.isVisible()
            data["knoppenbalk"] = self.btn_bar.isVisible()
            data["zijbalk"] = self.dir_tree.isVisible()
            for k in DEFAULT_THEME:
                v = self.theme.get(k)
                if v is not None and v != "":
                    data[k] = v
                else:
                    # leeg = terug naar standaard: oude waarde WISSEN,
                    # anders blijft een oude kleur na herstart actief
                    data.pop(k, None)
            if isinstance(self.theme.get("window_layout"), dict):
                data["window_layout"] = self.theme["window_layout"]
            else:
                data.pop("window_layout", None)
            data["left"] = self.left.save_state()
            data["right"] = self.right.save_state()
            with open(CONFIG_PATH, "w") as f:
                json.dump(data, f, indent=1)
        except OSError:
            pass

    def restore_settings(self):
        if not os.path.isfile(CONFIG_PATH):
            self._check_first_run_language()
            return
        try:
            with open(CONFIG_PATH) as f:
                data = json.load(f)
        except (OSError, ValueError):
            self._check_first_run_language()
            return

        lang = data.get("language")
        if lang in ("nl", "en"):
            set_language(lang)
        else:
            self._check_first_run_language()

        if isinstance(data.get("left"), list):
            self.left.restore_state(data["left"])
        if isinstance(data.get("right"), list):
            self.right.restore_state(data["right"])

    def closeEvent(self, event):
        self.save_settings()
        # Actieve overdracht netjes afbreken en de wachtrij legen, zodat
        # het venster niet wacht op een trage netwerk-upload.
        w = getattr(self, "worker", None)
        if w is not None:
            try:
                w.cancel()
            except RuntimeError:
                pass
        try:
            self._transfer_queue = []
        except AttributeError:
            pass
        dlg = getattr(self, "_transfer_dlg", None)
        if dlg is not None:
            try:
                dlg.close()
            except RuntimeError:
                pass
        super().closeEvent(event)
        # Vangnet: threads die vastzitten op een dode/trage netwerk-mount
        # mogen het afsluiten nooit blokkeren. Normale afsluiting is dan
        # al klaar; anders forceren we na 1,5 s het einde (instellingen
        # zijn op dit punt al bewaard).
        QTimer.singleShot(1500, lambda: os._exit(0))

    def _build_menubar(self):
        mb = self.menuBar()

        m_file = mb.addMenu(tr("&Bestand"))
        act_new_tab = QAction(tr("Nieuw tabblad") + "\tCtrl+T", self)
        act_new_tab.triggered.connect(
            lambda: self.focused_panel().add_tab(
                self.focused_panel().current_path,
                index=self.focused_panel().tabs.currentIndex() + 1,
            )
        )
        act_close_tab = QAction(tr("Tabblad sluiten") + "\tCtrl+W", self)
        act_close_tab.triggered.connect(self._close_active_tab)
        act_quit = QAction(tr("Afsluiten") + "\tF10", self)
        act_quit.triggered.connect(self.close)
        for a in (act_new_tab, act_close_tab):
            m_file.addAction(a)
        m_file.addSeparator()
        m_file.addAction(act_quit)

        m_edit = mb.addMenu(tr("&Bewerken"))
        act_nf = QAction(tr("Nieuwe map...") + "\tF7", self)
        act_nf.triggered.connect(lambda: self.focused_panel().new_folder())
        act_ren = QAction(tr("Hernoemen...") + "\tF2", self)
        act_ren.triggered.connect(lambda: self.focused_panel().rename_selected())
        act_del = QAction(tr("Naar prullenbak") + "\tDel", self)
        act_del.triggered.connect(self.delete_selected)
        for a in (act_nf, act_ren, act_del):
            m_edit.addAction(a)

        m_view = mb.addMenu(tr("&Beeld"))
        act_sidebar_v = QAction(tr("Zijbalk (mapboom) zichtbaar maken"), self)
        self.act_sidebar = act_sidebar_v
        act_sidebar_v.setCheckable(True)
        act_sidebar_v.setChecked(bool(load_ui_settings().get("zijbalk", True)))
        act_sidebar_v.toggled.connect(self.dir_tree.setVisible)
        m_view.addAction(act_sidebar_v)

        # Beheer-menu: archieven, splitsen, wissen, zoeken, synchroniseren
        m_manage = mb.addMenu(tr("&Beheer"))
        self.m_manage = m_manage
        if mutagen is not None:
            act_tags_menu = QAction(tr("🎵 Tag-editor openen..."), self)
            act_tags_menu.triggered.connect(
                lambda _checked=False: self.open_tag_editor())
            m_manage.addAction(act_tags_menu)
        if Renamer is not None:
            act_ren_mass = QAction(tr("✏ Massaal hernoemen..."), self)
            act_ren_mass.triggered.connect(
                lambda _checked=False: self.open_renamer())
            m_manage.addAction(act_ren_mass)
        if FtpDialog is not None:
            act_ftp = QAction(tr("📡 FTP / SFTP-client..."), self)
            act_ftp.triggered.connect(
                lambda _checked=False: self.open_ftp_client())
            m_manage.addAction(act_ftp)
        m_manage.addSeparator()
        act_root_here = QAction(tr("Map openen als root (nieuw venster)") + "\tCtrl+Alt+R", self)
        act_root_here.setShortcut("Ctrl+Alt+R")
        act_root_here.triggered.connect(self.open_root_here)
        m_manage.addAction(act_root_here)
        m_manage.addSeparator()
        act_arc_open = QAction(tr("Openen als archief..."), self)
        act_arc_open.triggered.connect(self.open_archive_dialog)
        act_arc_new = QAction(tr("Inpakken als archief (formaat + sterkte)..."), self)
        act_arc_new.triggered.connect(self.create_archive_selected)
        act_split = QAction(tr("Bestand splitsen..."), self)
        act_split.triggered.connect(self.split_selected)
        act_merge = QAction(tr("Bestandsdelen samenvoegen (.001)..."), self)
        act_merge.triggered.connect(self.merge_selected)
        act_dupes = QAction(tr("Dubbele bestanden zoeken..."), self)
        act_dupes.triggered.connect(self.find_duplicates)
        act_sync = QAction(tr("Mappen vergelijken..."), self)
        act_sync.triggered.connect(self.compare_folders)
        act_search = QAction(tr("🔍 Geavanceerd zoeken..."), self)
        act_search.triggered.connect(self.advanced_search)
        for a in (act_arc_open, act_arc_new, act_split, act_merge,
                  act_dupes, act_sync, act_search):
            m_manage.addAction(a)

        m_help = mb.addMenu(tr("&Help"))
        act_about = QAction(tr("Over Lopus"), self)
        act_about.triggered.connect(
            lambda: QMessageBox.about(
                self,
                tr("Over Lopus"),
                tr("Lopus — dual-pane bestandsbeheerder voor Linux.\n"
                   "Python + PyQt6."),
            )
        )
        m_help.addAction(act_about)

        # Opdrachten-menu: eigen scripts uit ~/.config/lopus/scripts
        self.m_scripts = mb.addMenu(tr("&Opdrachten"))
        self._rebuild_scripts_menu()

        # Instellingen-menu (na Help): beheert balken en knoppen
        self.m_buttons = mb.addMenu(tr("&Instellingen"))
        m_lang = self.m_buttons.addMenu(tr("Taal") + " / Language")
        act_nl = QAction("Nederlands 🇳🇱", self)
        act_en = QAction("English 🇬🇧", self)
        act_nl.triggered.connect(lambda: self.change_language("nl"))
        act_en.triggered.connect(lambda: self.change_language("en"))
        m_lang.addAction(act_nl)
        m_lang.addAction(act_en)
        self.m_view = m_view

    def _close_active_tab(self):
        p = self.focused_panel()
        p.close_tab(p.tabs.currentIndex())

    def _build_toolbar(self):
        settings = load_ui_settings()
        tb = QToolBar(tr("Functiebalk"))
        tb.setObjectName("FunctionToolBar")
        tb.setMovable(False)
        self.addToolBar(Qt.ToolBarArea.BottomToolBarArea, tb)
        self.func_bar = tb
        acts = [
            (tr("F5 Kopiëren →"), lambda: self.transfer_selected(False)),
            (tr("F6 Verplaatsen →"), lambda: self.transfer_selected(True)),
            (tr("F7 Nieuwe map"), lambda: self.focused_panel().new_folder()),
            (tr("F2 Hernoemen"), lambda: self.focused_panel().rename_selected()),
            (tr("Prullenbak"), self.delete_selected),
            (tr("F9 Verborgen bestanden"), self.toggle_hidden),
            (tr("F10 Afsluiten"), self.close),
        ]
        for label, slot in acts:
            tb.addAction(label).triggered.connect(slot)

        # Beeld-menu: verborgen bestanden, filterbalk en voorbeeldpaneel
        m_view = self.m_view
        act_hidden = QAction(tr("Verborgen bestanden tonen") + "\tF9", self)
        act_hidden.setCheckable(True)
        act_hidden.triggered.connect(self.toggle_hidden)
        self._act_hidden = act_hidden
        m_view.addAction(act_hidden)
        m_view.addSeparator()
        self.act_filter = QAction(tr("Filterbalk tonen") + "\tCtrl+F", self)
        self.act_filter.setCheckable(True)
        self.act_filter.toggled.connect(self.set_filter_visible)
        m_view.addAction(self.act_filter)
        self.act_preview = self.preview.toggleViewAction()
        self.act_preview.setText(tr("Voorbeeldpaneel tonen") + "\tF3")
        m_view.addAction(self.act_preview)
        m_view.addSeparator()
        # Boomweergave en compacte modus staan per paneel op de 🖼-knop
        # van de padbalk en dus niet (meer) globaal in dit menu.

        # Knoppenbalk: direct onder het hoofdmenu (bovenaan)
        btn_bar = QToolBar("Knoppen")
        btn_bar.setObjectName("ButtonBar")
        btn_bar.setMovable(False)
        btn_bar.setIconSize(QSize(20, 20))
        btn_bar.setMinimumHeight(34)
        self.addToolBar(Qt.ToolBarArea.TopToolBarArea, btn_bar)
        self.insertToolBarBreak(btn_bar)
        act_trash = QAction(tr("🗑 Prullenbak"), self)
        act_trash.setToolTip(tr("Prullenbak openen (herstellen / legen)"))
        act_trash.triggered.connect(self.open_trash)
        btn_bar.addAction(act_trash)
        act_hash_btn = QAction(tr("#️⃣ Checksum"), self)
        act_hash_btn.setToolTip(
            "SHA-256 van het geselecteerde bestand berekenen "
            "(resultaat komt op het klembord)")
        act_hash_btn.triggered.connect(self.checksum_selected)
        btn_bar.addAction(act_hash_btn)
        if mutagen is not None and TagEditor is not None:
            act_tags = QAction("🎵 Tag-editor", self)
            act_tags.setToolTip(
                tr("Muziekmap openen in de tag-editor (Mp3tag-stijl)"))
            act_tags.triggered.connect(
                lambda _checked=False: self.open_tag_editor())
            btn_bar.addAction(act_tags)
        self.btn_bar = btn_bar
        self._script_toolbar_actions = []
        self._rebuild_script_buttons()
        self.btn_bar = btn_bar

        # Instellingen-menu: functiebalk en knoppenbalk aan/uit
        act_tb = QAction(tr("Functiebalk zichtbaar maken"), self)
        self.act_tb = act_tb
        act_tb.setCheckable(True)
        act_tb.setChecked(bool(settings.get("functiebalk", True)))
        act_tb.toggled.connect(tb.setVisible)
        self.m_buttons.addAction(act_tb)
        self.m_buttons.addSeparator()
        act_bar = QAction(tr("Knoppenbalk zichtbaar maken"), self)
        self.act_bar = act_bar
        act_bar.setCheckable(True)
        act_bar.setChecked(bool(settings.get("knoppenbalk", True)))
        act_bar.toggled.connect(btn_bar.setVisible)
        self.m_buttons.addAction(act_bar)
        # Zijbalk-vink staat tegenwoordig in het Beeld-menu (self.act_sidebar
        # wordt daar al aangemaakt en verbonden)
        self.m_buttons.addSeparator()
        act_dsizes = QAction(tr("Mapgroottes berekenen (kolom Grootte)"), self)
        act_dsizes.setCheckable(True)
        act_dsizes.setChecked(bool(settings.get("map_groottes", False)))
        act_dsizes.toggled.connect(self.set_dir_sizes_enabled)
        self.m_buttons.addAction(act_dsizes)
        act_fopts = QAction(tr("Mapopties (kolommen per map)..."), self)
        act_fopts.triggered.connect(self.open_folder_options)
        self.m_buttons.addAction(act_fopts)
        self.m_buttons.addSeparator()
        act_look = QAction(tr("Weergave (lettertype, kleuren en thema's)..."), self)
        act_look.triggered.connect(self.open_appearance)
        self.m_buttons.addAction(act_look)
        self.m_buttons.addSeparator()
        act_export = QAction(tr("Instellingen exporteren..."), self)
        act_export.triggered.connect(self.export_settings)
        self.m_buttons.addAction(act_export)
        act_import = QAction(tr("Instellingen importeren..."), self)
        act_import.triggered.connect(self.import_settings)
        self.m_buttons.addAction(act_import)

        tb.setVisible(act_tb.isChecked())
        btn_bar.setVisible(act_bar.isChecked())
        self.dir_tree.setVisible(self.act_sidebar.isChecked())
        self.apply_bar_metrics()
        for a in (act_tb, act_bar, self.act_sidebar):
            a.toggled.connect(lambda _on: self.save_settings())

    def apply_bar_metrics(self):
        """Apply theme-configured icon sizes to the bars."""
        s = bar_icon_size(self.theme, "size_functiebalk")
        self.func_bar.setIconSize(QSize(s, s))
        s2 = bar_icon_size(self.theme, "size_knoppenbalk", default=20)
        self.btn_bar.setIconSize(QSize(s2, s2))

    def apply_bar_metrics(self):
        """Apply theme-configured icon sizes to the bars."""
        s = bar_icon_size(self.theme, "size_functiebalk")
        self.func_bar.setIconSize(QSize(s, s))
        s2 = bar_icon_size(self.theme, "size_knoppenbalk", default=20)
        self.btn_bar.setIconSize(QSize(s2, s2))

    # ---------- filter & preview ----------
    def set_filter_visible(self, visible):
        self.left.set_filter_visible(visible)
        self.right.set_filter_visible(visible)

    def toggle_preview(self):
        self.preview.setVisible(not self.preview.isVisible())
        if self.preview.isVisible():
            self._update_preview_for(self.focused_panel().view())

    PREVIEW_IMG_EXTS = {".png", ".jpg", ".jpeg", ".gif", ".bmp", ".webp", ".svg"}
    PREVIEW_VID_EXTS = {".mp4", ".mkv", ".avi", ".mov", ".webm", ".mpg",
                        ".mpeg", ".wmv", ".flv", ".m4v", ".ts"}
    PREVIEW_TEXT_MAX = 512 * 1024

    def _update_preview_for(self, view):
        from PyQt6.QtGui import QPixmap

        if not self.preview.isVisible():
            return
        if self._pv_player is not None:
            try:
                self._pv_player.stop()
            except Exception:  # noqa: BLE001
                pass
        sel = view.tree.selectedItems()
        self._pv_stack.setCurrentIndex(0)
        if len(sel) != 1:
            e = None
            cur = view.tree.currentItem()
            if cur:
                e = cur.data(COL_NAME, Qt.ItemDataRole.UserRole)
            if not e:
                self._pv_placeholder.setText(tr("Selecteer één bestand..."))
                return
        else:
            e = sel[0].data(COL_NAME, Qt.ItemDataRole.UserRole)
        if e is None or e["isdir"]:
            self._pv_placeholder.setText(
                f"📁 {e['name']}\n\n(map — geen voorbeeld)")
            return
        path = e["full"]
        ext = os.path.splitext(path)[1].lower()
        if ext in self.PREVIEW_IMG_EXTS:
            pm = QPixmap(path)
            if not pm.isNull():
                pm = pm.scaled(
                    max(self._pv_image.width(), 200),
                    max(self._pv_image.height(), 300),
                    Qt.AspectRatioMode.KeepAspectRatio,
                    Qt.TransformationMode.SmoothTransformation,
                )
                self._pv_image.setPixmap(pm)
                self._pv_stack.setCurrentWidget(self._pv_image)
                return
        if ext in self.PREVIEW_VID_EXTS and self._pv_player is not None:
            try:
                self._pv_player.stop()
                from PyQt6.QtCore import QUrl
                self._pv_player.setSource(QUrl.fromLocalFile(path))
                self._pv_stack.setCurrentWidget(self._pv_vw)
                self._pv_player.play()
                return
            except Exception:  # noqa: BLE001
                pass
        try:
            size = os.path.getsize(path)
            with open(path, "rb") as f:
                raw = f.read(min(size, self.PREVIEW_TEXT_MAX))
            if b"\x00" in raw[:4096]:
                raise ValueError("binair bestand")
            text = raw.decode("utf-8", errors="replace")
            if size > self.PREVIEW_TEXT_MAX:
                text += f"\n\n... (afgekapt, totaal {human_size(size)})"
            self._pv_text.setPlainText(text)
            self._pv_stack.setCurrentWidget(self._pv_text)
            return
        except (OSError, ValueError):
            pass
        self._pv_placeholder.setText(
            f"{e['name']}\n\nGeen voorbeeld beschikbaar voor dit type.")

    def _toggle_pv_playback(self):
        if self._pv_player is None:
            return
        try:
            if self._pv_player.playbackState() \
                    == self._pv_player.PlaybackState.PlayingState:
                self._pv_player.pause()
            else:
                self._pv_player.play()
        except Exception:  # noqa: BLE001
            pass

    def set_display_options(self, key, on, side=None):
        """Weergave-instelling; met `side` alleen voor dat paneel.

        Per-paneel instellingen worden opgeslagen als `<key>_<side>`;
        zonder side (Beeld-menu: boomweergave/compact) gelden beide."""
        data = load_ui_settings()
        sfx = f"_{side}" if side else ""
        if key == "view_thumb_size":
            data[f"{key}{sfx}"] = int(on)
        elif key == "view_group":
            data[f"{key}{sfx}"] = str(on or "")
        else:
            data[f"{key}{sfx}"] = bool(on)
        self._write_settings(data)
        attr = {"view_thumbnails": "thumbnails",
                "view_flat": "flat",
                "view_compact": "compact",
                "view_thumb_size": "thumb_size",
                "view_thumb_info": "thumb_info",
                "view_colors": "color_files",
                "view_group": "group_mode"}.get(key)
        panels = [self.left, self.right]
        if side:
            panels = [self.left if side == "left" else self.right]
        for panel in panels:
            for i in range(panel.tabs.count()):
                wv = panel.tabs.widget(i)
                if attr:
                    if key == "view_thumb_size":
                        setattr(wv, attr, int(on))
                    elif key == "view_group":
                        setattr(wv, attr, str(on or ""))
                    else:
                        setattr(wv, attr, bool(on))
                if key == "view_thumb_size":
                    wv._thumb_cache.clear()
                wv.apply_display_options()
        # Lijst opnieuw laden bij wijzigingen die de inhoud/icoonts raken
        if key in ("view_flat", "view_thumbnails", "view_thumb_size",
                   "view_colors", "view_group"):
            for panel in panels:
                panel.refresh()
        # Weergave-knoppen van de betreffende pane(len) synchroon houden
        for panel in panels:
            for i in range(panel.tabs.count()):
                wv = panel.tabs.widget(i)
                if hasattr(wv, "_build_viewmode_menu"):
                    wv._build_viewmode_menu()

    def set_dir_sizes_enabled(self, on):
        data = load_ui_settings()
        data["map_groottes"] = bool(on)
        self._write_settings(data)
        if on:
            self.left.compute_dir_sizes()
            self.right.compute_dir_sizes()

    def open_folder_options(self):
        dlg = FolderOptionsDialog(
            self.focused_panel().current_path, self, self)
        dlg.exec()

    # ---------- instellingen exporteren / importeren ----------
    def _qt_file_dialog(self, title, startdir, mode):
        """Qt-bestandsdialoog i.p.v. KDE-native: sneller en zonder
        KIO-waarschuwingen ('No node found for item that was just removed')."""
        dlg = QFileDialog(
            self, title, os.path.expanduser(startdir), "JSON (*.json)")
        dlg.setOption(QFileDialog.Option.DontUseNativeDialog, True)
        dlg.setFileMode(mode)
        return dlg

    def export_settings(self):
        """Sla alle instellingen (tabs.json) op in een bestand."""
        dlg = self._qt_file_dialog(
            "Instellingen exporteren", "~/lopus_instellingen.json",
            QFileDialog.FileMode.AnyFile)
        if not dlg.exec():
            return
        dest = dlg.selectedFiles()[0]
        if not dest:
            return
        try:
            shutil.copyfile(CONFIG_PATH, dest)
            # Overzicht van wat erin zit
            data = load_ui_settings()
            kleuren = [k for k in DEFAULT_THEME if k.startswith("color_")
                       and data.get(k)]
            themas = list(data.get("themes", {}).keys())
            tabs = ("ja" if (data.get("left") or data.get("right"))
                    else "nee")
            layout = "ja" if data.get("window_layout") else "nee"
            QMessageBox.information(
                self, APP_NAME,
                "Instellingen geëxporteerd naar:\n" + dest +
                "\n\nInhoud:\n"
                f"• Kleuren: {len(kleuren)} aangepast"
                + (f" ({', '.join(kleuren)})" if kleuren else "") +
                f"\n• Opgeslagen thema's: {len(themas)}"
                + (f" ({', '.join(themas)})" if themas else "") +
                f"\n• Balkhoogtes: "
                f"{sum(1 for k in DEFAULT_THEME if k.startswith('size_') and data.get(k))}"
                f"\n• Vensterlayout: {layout}"
                f"\n• Tabbladen: {tabs}"
                "\n\nTip: je eigen scripts staan apart in\n" + SCRIPTS_DIR)
        except OSError as e:
            QMessageBox.warning(self, APP_NAME, str(e))

    def import_settings(self):
        """Instellingen uit een exportbestand terugzetten."""
        dlg = self._qt_file_dialog(
            "Instellingen importeren", "~",
            QFileDialog.FileMode.ExistingFile)
        if not dlg.exec():
            return
        src = dlg.selectedFiles()[0]
        if not src:
            return
        try:
            with open(src) as f:
                data = json.load(f)
            if not isinstance(data, dict):
                raise ValueError("Geen geldig Lopus-instellingenbestand")
        except (OSError, ValueError) as e:
            QMessageBox.warning(self, APP_NAME,
                                f"Kan bestand niet lezen:\n{e}")
            return
        kleuren = [k for k in DEFAULT_THEME if k.startswith("color_")
                   and data.get(k)]
        r = QMessageBox.question(
            self, APP_NAME,
            "Instellingen importeren?\n\nJe huidige instellingen worden "
            "overschreven."
            + (f"\n\nDit bestand bevat {len(kleuren)} aangepaste kleuren."
               if kleuren else ""))
        if r != QMessageBox.StandardButton.Yes:
            return
        try:
            os.makedirs(os.path.dirname(CONFIG_PATH), exist_ok=True)
            with open(CONFIG_PATH, "w") as f:
                json.dump(data, f, indent=1)
            # Direct toepassen, zonder herstart: theme + balken + layout
            self.theme = current_theme()
            apply_theme(self.theme)
            self.apply_bar_metrics()
            lay = data.get("window_layout")
            if isinstance(lay, dict):
                self.apply_layout(lay)
            QMessageBox.information(
                self, APP_NAME,
                "Instellingen geïmporteerd en toegepast.\n"
                "(Tabbladherstel zie je na de volgende start)")
        except OSError as e:
            QMessageBox.warning(self, APP_NAME, str(e))

    def open_appearance(self):
        dlg = AppearanceDialog(self.theme, self)
        if dlg.exec():
            self.theme = dlg.result_theme()
            apply_theme(self.theme)
            self.apply_bar_metrics()
            if isinstance(self.theme.get("window_layout"), dict):
                self.apply_layout(self.theme["window_layout"])
            self.save_settings()

    def _write_settings(self, data):
        try:
            os.makedirs(os.path.dirname(CONFIG_PATH), exist_ok=True)
            with open(CONFIG_PATH, "w") as f:
                json.dump(data, f, indent=1)
        except OSError:
            pass

    def open_trash(self):
        dlg = TrashDialog(self)
        dlg.exec()
        # Panels can change if items were restored into them
        self.left.refresh()
        self.right.refresh()

    def checksum_selected(self):
        """SHA-256 of the single selected file -> dialog + clipboard."""
        panel = self.focused_panel()
        paths = [p for p in panel.selected_paths() if os.path.isfile(p)]
        if len(paths) != 1:
            self.set_status(tr("Checksum: selecteer één bestand"), 4000)
            return
        path = paths[0]
        self.set_status(f"SHA-256 berekenen: {os.path.basename(path)}")

        worker = FuncWorker(lambda: (path, sha256_of(path)))

        def show_hash(res):
            if isinstance(res, tuple):
                QApplication.clipboard().setText(res[1])
                QMessageBox.information(
                    self, "SHA-256",
                    f"Bestand:\n{res[0]}\n\nSHA-256:\n{res[1]}\n\n"
                    "(checksum is naar het klembord gekopieerd)")
            else:
                QMessageBox.warning(self, "SHA-256", str(res))

        worker.result_ready.connect(show_hash)
        worker.start()

    def open_tag_editor(self, path=None, force=False):
        """Open de Mp3tag-stijl editor met de huidige map van het paneel
        (of een opgegeven map/bestandsmap)."""
        if mutagen is None:
            QMessageBox.warning(
                self, APP_NAME,
                tr("De tag-editor heeft de Python-module 'mutagen' nodig.\n"
                   "Installeer die met:\n"
                   "  pip install --user --break-system-packages mutagen"))
            return
        if TagEditor is None:
            QMessageBox.warning(
                self, APP_NAME,
                tr("tageditor.py ontbreekt naast lopus.py."))
            return
        if path is None:
            path = self.focused_panel().current_path
        if not isinstance(path, str) or not path:
            return
        if os.path.isfile(path):
            path = os.path.dirname(path)
        dlg = getattr(self, "_tag_editor", None)
        fresh = dlg is None or not dlg.isVisible()
        if fresh:
            dlg = TagEditor(self)
            self._tag_editor = dlg
        dlg.show()
        dlg.raise_()
        dlg.activateWindow()
        if fresh or force:
            dlg.load_folder(path)

    def open_ftp_client(self):
        """Open de FTP/SFTP-client (ftp.py); start altijd in de home-map."""
        if FtpDialog is None:
            QMessageBox.warning(self, APP_NAME,
                                tr("ftp.py ontbreekt naast lopus.py."))
            return
        import traceback
        try:
            dlg = FtpDialog(self, start_local=os.path.expanduser("~"))
        except Exception as e:
            err = traceback.format_exc()
            print(err, file=sys.stderr)
            log_error("FTP-client kon niet openen", exc=e)
            QMessageBox.critical(
                self, APP_NAME,
                tr("FTP-client kon niet openen:") + "\n" + err[-1500:])
            return
        dlg.exec()


    def open_renamer(self, path=None, files=None):
        """Open het massaal-hernoem-venster voor de huidige map (of een
        opgegeven map / bestandsselectie)."""
        if Renamer is None:
            QMessageBox.warning(self, APP_NAME,
                                tr("renamer.py ontbreekt naast lopus.py."))
            return
        if files is None and path is None:
            sel_fn = getattr(self.focused_panel(), "selected_paths", None)
            if callable(sel_fn):
                sel = sel_fn()
                if len(sel) > 1:
                    files = [p for p in sel if os.path.isfile(p)]
                elif sel:
                    path = sel[0]
        if not files:
            if path is None:
                path = self.focused_panel().current_path
            if not isinstance(path, str) or not path:
                return
            if os.path.isfile(path):
                path = os.path.dirname(path)
        dlg = Renamer(self, folder=path, files=files)
        dlg.exec()

    def focused_panel(self):
        widget = QApplication.focusWidget()
        while widget is not None:
            if isinstance(widget, FilePanel):
                return widget
            widget = widget.parentWidget()
        return self.left

    def remap_stale_tabs(self):
        """Zet open tabbladen die nog naar een oud Lopus-mountpad wijzen
        (bijv. /mnt/lopus-nfs/...) over naar de huidige koppeling van
        dezelfde share — direct na het verbinden, zonder herstart."""
        for panel in (self.left, self.right):
            for i in range(panel.tabs.count()):
                v = panel.tabs.widget(i)
                if v is None:
                    continue
                new = remap_stale_lopus_mount(v.current_path)
                if new != v.current_path:
                    v.load_path(new)

    def keyPressEvent(self, event):
        key = event.key()
        mods = event.modifiers()
        if mods & Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_T:
            p = self.focused_panel()
            p.add_tab(p.current_path, index=p.tabs.currentIndex() + 1)
            return
        if mods & Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_W:
            p = self.focused_panel()
            p.close_tab(p.tabs.currentIndex())
            return
        if mods & Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_F:
            vis = not self.left.filter_row.isVisible()
            self.set_filter_visible(vis)
            if vis:
                self.focused_panel().view().filter_edit.setFocus()
            return
        if key == Qt.Key.Key_F3:
            self.toggle_preview()
            return
        if mods & Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_C:
            self.clipboard_copy()
            return
        if mods & Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_X:
            self.clipboard_cut()
            return
        if mods & Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_V:
            self.clipboard_paste()
            return
        if mods & Qt.KeyboardModifier.ControlModifier:
            super().keyPressEvent(event)
            return
        if key == Qt.Key.Key_F5:
            self.transfer_selected(False)
        elif key == Qt.Key.Key_F6:
            self.transfer_selected(True)
        elif key == Qt.Key.Key_F7:
            self.focused_panel().new_folder()
        elif key == Qt.Key.Key_F2:
            self.focused_panel().rename_selected()
        elif key in (Qt.Key.Key_Delete, Qt.Key.Key_F8):
            self.delete_selected()
        elif key == Qt.Key.Key_F9:
            self.toggle_hidden()
        elif key == Qt.Key.Key_F10:
            self.close()
        elif key == Qt.Key.Key_Backspace and not isinstance(
            QApplication.focusWidget(), QLineEdit
        ):
            self.focused_panel().go_up()
        else:
            super().keyPressEvent(event)

    def transfer_selected(self, move):
        src_panel = self.focused_panel()
        dst_panel = self.right if src_panel is self.left else self.left
        srcs = src_panel.selected_paths()
        if not srcs:
            self.set_status(tr("Geen selectie"), 3000)
            return
        pairs = [(src, os.path.join(dst_panel.current_path,
                                    os.path.basename(src.rstrip("/"))))
                 for src in srcs]
        jobs, cancelled = resolve_conflicts(pairs, self)
        if cancelled:
            self.set_status("Overdracht afgebroken", 3000)
            return
        if not jobs:
            self.set_status(tr("Niets te doen (alles overgeslagen)"), 3000)
            return
        self._run_transfer(jobs, move)

    # ---------- clipboard (Ctrl+C / X / V) ----------
    def clipboard_copy(self):
        self.clipboard = {"mode": "copy",
                          "paths": self.focused_panel().selected_paths()}
        self.set_status(
            f"{len(self.clipboard['paths'])} item(s) gekopieerd", 3000)

    def clipboard_cut(self):
        self.clipboard = {"mode": "cut",
                          "paths": self.focused_panel().selected_paths()}
        self.set_status(
            f"{len(self.clipboard['paths'])} item(s) geknipt", 3000)

    def clipboard_paste(self):
        paths = self.clipboard.get("paths") or []
        if not paths:
            self.set_status(tr("Klembord is leeg"), 3000)
            return
        dest = self.focused_panel().current_path
        move = self.clipboard.get("mode") == "cut"
        pairs = []
        for src in paths:
            if not os.path.exists(src):
                continue
            dst = os.path.join(dest, os.path.basename(src.rstrip("/")))
            if os.path.abspath(src) == os.path.abspath(dst):
                if not move:
                    # duplicaat in dezelfde map: altijd hernoemen
                    pairs.append((src, unique_path(dst)))
                # else: moving onto itself: nothing to do
            else:
                pairs.append((src, dst))
        if not pairs:
            self.set_status(tr("Niets te plakken"), 3000)
            return
        jobs, cancelled = resolve_conflicts(pairs, self)
        if cancelled:
            self.set_status(tr("Plakken afgebroken"), 3000)
            return
        if not jobs:
            self.set_status(tr("Niets geplakt (alles overgeslagen)"), 3000)
            return
        was_cut = move and self.clipboard.get("mode") == "cut"

        def after(ok, err):
            if ok and was_cut:
                self.clipboard = {"mode": None, "paths": []}

        self._run_transfer(jobs, move, done_extra=after)

    def _run_transfer(self, jobs, move, done_extra=None):
        """Zet een overdracht klaar; loopt acties netjes in de wachtrij
        zodat er altijd maar één kopieer-/verplaatsactie tegelijk draait
        (elk met eigen pauze/hervatten/annuleren)."""
        if getattr(self, "_transfer_active", False):
            queue = getattr(self, "_transfer_queue", [])
            queue.append((jobs, move, done_extra))
            self._transfer_queue = queue
            self.set_status(
                f"⏳ In de wachtrij geplaatst ({len(queue)} wachtend) — "
                "deze start als de huidige overdracht klaar is", 6000)
            return
        self._transfer_active = True
        self._run_transfer_now(jobs, move, done_extra)

    def _start_next_transfer(self):
        """Neem de volgende actie uit de wachtrij (na afloop van de vorige)."""
        queue = getattr(self, "_transfer_queue", [])
        if queue:
            jobs, move, done_extra = queue.pop(0)
            if queue:
                self.set_status(
                    f"▶ Wachtrij: volgende overdracht gestart "
                    f"({len(queue)} daarna nog)", 5000)
            self._run_transfer_now(jobs, move, done_extra)
        else:
            self._transfer_active = False

    def _run_transfer_now(self, jobs, move, done_extra=None):
        dlg = QDialog(self)
        dlg.setWindowTitle("Verplaatsen" if move else "Kopiëren")
        lay = QVBoxLayout(dlg)
        lbl = QLabel(tr("Bezig..."))
        bar = QProgressBar()
        bar.setRange(0, 100)
        bar.setTextVisible(True)
        bar.setFormat("%p%")
        bar.setMinimumHeight(20)
        bar.setStyleSheet(
            "QProgressBar { border: 1px solid #888; border-radius: 3px;"
            " background: #dddddd; text-align: center; color: #111; }"
            " QProgressBar::chunk { background-color: #3a5a8c; }")
        info_lbl = QLabel("")
        cancel_btn = QPushButton(tr("Annuleren"))
        pause_btn = QPushButton(tr("⏸ Pauze"))
        pause_btn.setCheckable(True)

        def toggle_pause(checked):
            if checked:
                self.worker.pause()
                pause_btn.setText(tr("▶ Hervatten"))
            else:
                self.worker.resume()
                pause_btn.setText(tr("⏸ Pauze"))

        pause_btn.toggled.connect(toggle_pause)
        row = QHBoxLayout()
        row.addWidget(pause_btn)
        row.addStretch(1)
        row.addWidget(cancel_btn)
        lay.addWidget(lbl)
        lay.addWidget(bar)
        lay.addWidget(info_lbl)
        lay.addLayout(row)

        verb = "Verplaatsen" if move else "Kopiëren"

        self.worker = CopyWorker(jobs, move)
        self._transfer_dlg = dlg
        self.worker.progress.connect(
            lambda v, n: (
                bar.setValue(v),
                lbl.setText(n),
                self.set_status(f"{verb} {v}% — {n}", 5000),
            )
        )
        self.worker.info.connect(info_lbl.setText)
        cancel_btn.clicked.connect(
            lambda: (self.worker.cancel(),
                     cancel_btn.setEnabled(False),
                     cancel_btn.setText(tr("Annuleren...")))
        )

        def on_done(ok, err):
            dlg.close()
            if err == CopyWorker.CANCELLED_MSG:
                self.set_status("Overdracht afgebroken", 4000)
            elif not ok:
                QMessageBox.warning(self, APP_NAME, f"Fouten:\n{err}")
            else:
                self.set_status(f"{verb} voltooid", 3000)
            if done_extra:
                done_extra(ok and err != CopyWorker.CANCELLED_MSG, err)
            self.left.refresh()
            self.right.refresh()
            self._start_next_transfer()

        self.worker.finished_ok.connect(on_done)
        self.worker.start()
        # Niet-modaal: je kunt doorwerken in het hoofdvenster en extra
        # overdrachten starten; die komen netjes in de wachtrij.
        dlg.show()

    # ---------- scripting-interface (eigen opdrachten) ----------
    def _rebuild_scripts_menu(self):
        self.m_scripts.clear()
        scripts = load_user_scripts()
        if not scripts:
            act = self.m_scripts.addAction(tr("(geen scripts gevonden)"))
            act.setEnabled(False)
        for s in scripts:
            act = self.m_scripts.addAction(tr(s["name"]))
            act.setToolTip(tr(s["tooltip"]))
            act.triggered.connect(
                lambda _c=False, sc=s: self.run_user_script(sc))
        self.m_scripts.addSeparator()
        act_reload = self.m_scripts.addAction(tr("🔄 Opdrachten herladen"))
        act_reload.triggered.connect(self.reload_user_scripts)
        act_open = self.m_scripts.addAction(tr("📁 Scripts-map openen"))
        act_open.triggered.connect(self.open_scripts_dir)
        act_doc = self.m_scripts.addAction(tr("❓ Hoe schrijf ik een script?"))
        act_doc.triggered.connect(self.show_script_help)

    def run_user_script(self, script):
        """Voer een gebruikersscript uit met (main, panel)."""
        try:
            script["module"].run(self, self.focused_panel())
        except Exception as e:  # noqa: BLE001
            log_error(f"script '{script['name']}' faalde", e)
            QMessageBox.warning(
                self, APP_NAME,
                f"Script-fout in '{script['name']}':\n{e}")

    def reload_user_scripts(self):
        self._rebuild_scripts_menu()
        self._rebuild_script_buttons()
        self.set_status(tr("Opdrachten herladen"), 3000)

    def open_scripts_dir(self):
        ensure_scripts_dir()
        subprocess.Popen(["xdg-open", SCRIPTS_DIR])

    def show_script_help(self):
        ensure_scripts_dir()
        QMessageBox.information(
            self, tr("Eigen opdrachten schrijven"),
            tr("Zet een .py-bestand in:\n")
            + f"{SCRIPTS_DIR}\n\n"
            + tr("Een script definieert:\n"
            "    def run(main, panel):\n"
            "        ...\n\n"
            "  main  = het hoofdvenster\n"
            "  panel = het actieve paneel\n\n"
            "Voorbeeld:\n"
            "    def run(main, panel):\n"
            '        main.set_status(panel.current_path, 4000)\n\n'
            "Optioneel bovenin het bestand:\n"
            '    TITLE = "Naam in het menu"\n'
            "    TOOLBAR = True   (ook een knop in de knoppenbalk)\n\n")
            + tr("Gebruik '🔄 Opdrachten herladen' na wijzigingen.\n"
               "Er staat een werkend voorbeeld in de scripts-map."))

    def _rebuild_script_buttons(self):
        """Eigen knoppen (scripts met TOOLBAR = True) in de knoppenbalk."""
        for act in getattr(self, "_script_toolbar_actions", []):
            self.btn_bar.removeAction(act)
        self._script_toolbar_actions = []
        for s in load_user_scripts():
            if not s["toolbar"]:
                continue
            act = QAction(tr(s["name"]), self)
            act.setToolTip(s["tooltip"])
            act.triggered.connect(
                lambda _c=False, sc=s: self.run_user_script(sc))
            self.btn_bar.addAction(act)
            self._script_toolbar_actions.append(act)

    # ---------- beheer-gereedschap ----------
    def add_netshare_dialog(self):
        """Netwerkshare toevoegen (opent de zijbalk-dialoog)."""
        self.dir_tree._add_netshare_dialog()

    def open_root_here(self):
        """Start een tweede Lopus als root op de huidige map."""
        err = open_root_lopus(self.focused_panel().current_path)
        if err:
            QMessageBox.warning(self, APP_NAME, err)

    def open_archive_dialog(self, path=None):
        p = path or (self.focused_panel().selected_paths() or [None])[0]
        if p and is_archive(p):
            ArchiveDialog(p, self).exec()

    def create_archive_selected(self):
        srcs = self.focused_panel().selected_paths()
        if not srcs:
            return
        base = os.path.basename(srcs[0].rstrip("/")) or "archief"
        dlg = ArchiveCreateDialog(base, self, sevenz_ok=_7z_available())
        if dlg.exec() != QDialog.DialogCode.Accepted:
            return
        name = dlg.result_name()
        if not name:
            return
        dest = os.path.join(self.focused_panel().current_path, name)
        if os.path.exists(dest):
            QMessageBox.warning(self, APP_NAME, f"Bestaat al:\n{dest}")
            return
        level = dlg.result_level()
        # Grote selecties op de achtergrond (met statusmelding)
        self.set_status(f"⏳ Inpakken als {name} (niveau {level})...", 0)
        self._archive_thread = FuncThread(
            lambda: create_archive(srcs, dest, level))

        def on_arch_done(err):
            if isinstance(err, Exception):
                self.set_status(tr("Inpakken mislukt"), 4000)
                QMessageBox.warning(self, APP_NAME, str(err))
            else:
                self.set_status(f"Archief gemaakt: {name}", 4000)
            self.focused_panel().refresh()

        self._archive_thread.done.connect(on_arch_done)
        self._archive_thread.finished.connect(
            self._archive_thread.deleteLater)
        self._archive_thread.start()

    def split_selected(self):
        srcs = self.focused_panel().selected_paths()
        if len(srcs) != 1 or not os.path.isfile(srcs[0]):
            self.set_status(tr("Splitsen: selecteer één bestand"), 3000)
            return
        size, ok = QInputDialog.getInt(
            self, "Bestand splitsen", "Grootte per deel (MB):", 10,
            1, 100000)
        if not ok:
            return
        try:
            parts = split_file(srcs[0], size)
            self.set_status(f"Gesplitst in {len(parts)} delen", 4000)
        except OSError as e:
            QMessageBox.warning(self, APP_NAME, str(e))
        self.focused_panel().refresh()

    def merge_selected(self):
        srcs = self.focused_panel().selected_paths()
        if len(srcs) != 1 or not srcs[0].endswith(".001"):
            self.set_status(tr("Samenvoegen: selecteer het .001-deel"), 3000)
            return
        try:
            base = merge_parts(srcs[0])
            self.set_status(f"Samengevoegd: {os.path.basename(base)}", 4000)
        except (OSError, ValueError) as e:
            QMessageBox.warning(self, APP_NAME, str(e))
        self.focused_panel().refresh()

    def find_duplicates(self):
        dlg = DuplicateFinderDialog(
            self.focused_panel().current_path, self,
            trash_fn=trash_paths)
        dlg.exec()
        self.focused_panel().refresh()

    def compare_folders(self):
        dlg = SyncDialog(self.left.current_path, self.right.current_path,
                         self, run_transfer=self._run_transfer)
        dlg.exec()
        self.left.refresh()
        self.right.refresh()

    def advanced_search(self):
        panel = self.focused_panel()

        def open_dir(d):
            panel.load_path(d)

        SearchDialog(panel.current_path, self, open_dir_cb=open_dir).exec()

    def delete_selected(self):
        panel = self.focused_panel()
        paths = panel.selected_paths()
        if not paths:
            return
        names = "\n".join(os.path.basename(p) for p in paths[:8])
        if len(paths) > 8:
            names += f"\n... en {len(paths) - 8} meer"
        r = QMessageBox.question(
            self,
            "Naar prullenbak",
            f"Deze {len(paths)} item(s) naar de prullenbak verplaatsen?\n\n{names}",
        )
        if r != QMessageBox.StandardButton.Yes:
            return
        if any(is_remote_path(p) for p in paths):
            # Netwerk-mount: gio trash kan lang duren -> achtergrondthread
            self.set_status(f"⏳ Verwijderen op netwerkmap ({len(paths)} item(s))...", 0)
            self._trash_thread = FuncThread(lambda: trash_paths(paths))
            panel_ref = panel

            def on_trash_done(failed):
                if isinstance(failed, Exception):
                    QMessageBox.warning(self, APP_NAME, str(failed))
                elif failed:
                    QMessageBox.warning(
                        self, APP_NAME,
                        "Niet gelukt:\n"
                        + "\n".join(f"{p}: {err}" for p, err in failed),
                    )
                self.set_status(tr("Verwijderen klaar"), 3000)
                panel_ref.refresh()

            self._trash_thread.done.connect(on_trash_done)
            self._trash_thread.finished.connect(self._trash_thread.deleteLater)
            self._trash_thread.start()
            return
        failed = trash_paths(paths)
        if failed:
            QMessageBox.warning(
                self,
                APP_NAME,
                "Niet gelukt:\n"
                + "\n".join(f"{p}: {err}" for p, err in failed),
            )
        panel.refresh()

    def toggle_hidden(self):
        show = not self.left.show_hidden
        self.left.set_show_hidden(show)
        self.right.set_show_hidden(show)


def main():
    app = QApplication(sys.argv)
    app.setApplicationName(APP_NAME)
    # Wayland: koppel het venster aan lopus.desktop, zodat het juiste
    # icoon in de taakbalk/dock wordt getoond (i.p.v. een standaard-W).
    app.setDesktopFileName("lopus")
    app.setStyle("Fusion")
    # Ctrl+C in de terminal moet het proces ALTIJD direct kunnen killen,
    # ook als de main thread in een C-loop van Qt vastzit.
    import signal
    try:
        signal.signal(signal.SIGINT, signal.SIG_DFL)
    except (ValueError, OSError):
        pass
    # Programma-icoon (SVG naast het script of geïnstalleerd)
    for icon_path in (
            os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "packaging", "lopus.svg"),
            os.path.expanduser("~/.local/share/lopus/lopus.svg"),
    ):
        if os.path.isfile(icon_path):
            app.setWindowIcon(QIcon(icon_path))
            break

    def excepthook(et, ev, tb):
        log_error("Onverwachte fout", exc=ev)
        sys.__excepthook__(et, ev, tb)

    sys.excepthook = excepthook

    win = MainWindow()
    win.show()
    # Optionele startmap (bijv. via "Openen als root" of lopus <map>)
    if len(sys.argv) > 1 and os.path.isdir(os.path.abspath(sys.argv[1])):
        win.focused_panel().load_path(os.path.abspath(sys.argv[1]))
    code = app.exec()
    # Vangnet: threads die vastzitten op een dode/trage netwerk-mount
    # (bijv. listdir op een hangende rclone-mount) mogen het afsluiten
    # nooit blokkeren. Instellingen zijn al bewaard in closeEvent.
    os._exit(code)


if __name__ == "__main__":
    main()





