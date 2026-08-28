"""Tag-editor voor Lopus.

Rijen = muziekbestanden, kolommen = tags (aan/uit vinkbaar); submappen
zijn rijen in de lijst (dubbelklik = naar binnen); grote mappen (ook met
submappen) worden op de achtergrond geladen; linkerpaneel met bulk-velden
voor de selectie en een cover-editor.
Vereist: pip install --user --break-system-packages mutagen
"""
import json
import os
import re
import subprocess
import time

from PyQt6.QtCore import QThread, Qt, QTimer, pyqtSignal, QSize
from PyQt6.QtGui import QAction, QColor, QPalette, QPixmap, QIcon
from PyQt6.QtWidgets import (
    QAbstractItemView,
    QComboBox,
    QDialog,
    QGroupBox,
    QHBoxLayout,
    QHeaderView,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QCheckBox,
    QPushButton,
    QRadioButton,
    QMenu,
    QMenuBar,
    QSplitter,
    QStyledItemDelegate,
    QTableWidget,
    QTableWidgetItem,
    QToolButton,
    QVBoxLayout,
    QWidget,
)

try:
    from translations import tr
except ImportError:
    def tr(s): return s


try:
    import mutagen
    from mutagen.flac import Picture as _FlacPicture
    # 'comment' zit niet in EasyID3's standaard-sleutels: registreren zodat
    # Commentaar-tagging werkt voor alle formaten.
    from mutagen.easyid3 import EasyID3
    EasyID3.RegisterTextKey("comment", "COMM")
except ImportError:  # pragma: no cover
    mutagen = None
    _FlacPicture = None

from PyQt6.QtCore import QTimer

# Muziekextensies: normaal overgenomen uit lopus (AUDIO_EXTS); deze
# fallback werkt ook als tageditor.py los wordt gebruikt.
try:
    from lopus import AUDIO_EXTS  # noqa: F401
except Exception:  # noqa: BLE001
    AUDIO_EXTS = {".mp3", ".flac", ".ogg", ".oga", ".opus", ".m4a", ".m4b",
                  ".aac", ".wav", ".wma", ".wv", ".ape"}

_CFG_FILE = os.path.expanduser("~/.config/lopus/tageditor.json")

# Kolommen: (sleutel, titel, bewerkbaar, standaard zichtbaar)
COLUMNS = [
    ("file", tr("Bestandsnaam"), True, True),
    ("map", tr("Map"), False, True),
    ("title", tr("Titel"), True, True),
    ("artist", tr("Artiest"), True, True),
    ("album", tr("Album"), True, True),
    ("albumartist", tr("Albumartiest"), True, True),
    ("track", tr("Nr"), True, True),
    ("disc", tr("Schijfnummer"), True, False),
    ("year", tr("Jaar"), True, True),
    ("genre", tr("Genre"), True, True),
    ("comment", tr("Commentaar"), True, False),
    ("composer", tr("Componist"), True, False),
    ("bpm", tr("BPM"), True, False),
    ("cover", tr("Hoesje"), False, True),
    ("codec", tr("Codec"), False, False),
    ("bitrate", tr("Bitrate"), False, False),
    ("samplerate", tr("Frequentie"), False, False),
    ("length", tr("Lengte"), False, True),
    ("size", tr("Grootte"), False, False),
    ("modified", tr("Gewijzigd"), False, False),
]
EDITABLE_KEYS = [k for k, _t, ed, _v in COLUMNS if ed]

# Bulk-velden in het linkerpaneel: (sleutel, label, breedte-variant)
BULK_FIELDS = [
    ("title", tr("Titel"), "wide"),
    ("artist", tr("Artiest"), "wide"),
    ("album", tr("Album"), "wide"),
    ("year", tr("Jaar"), "small"),
    ("track", tr("Nr"), "small"),
    ("genre", tr("Genre"), "small"),
    ("comment", tr("Commentaar"), "wide"),
    ("albumartist", tr("Albumartiest"), "wide"),
    ("composer", tr("Componist"), "wide"),
    ("disc", tr("Schijfnummer"), "small"),
    ("bpm", tr("BPM"), "small"),
]


def _load_cfg():
    try:
        with open(_CFG_FILE) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def _save_cfg(cfg):
    try:
        os.makedirs(os.path.dirname(_CFG_FILE), exist_ok=True)
        with open(_CFG_FILE, "w") as f:
            json.dump(cfg, f)
    except OSError:
        pass


def _fmt_len(secs):
    try:
        secs = int(float(secs))
    except (TypeError, ValueError):
        return ""
    return f"{secs // 60}:{secs % 60:02d}"


def get_cover(path):
    """Cover-art als bytes (of None) uit een muziekbestand halen."""
    try:
        mf = mutagen.File(path)
        if mf is None:
            return None
        if hasattr(mf, "pictures") and mf.pictures:      # FLAC/OGG
            return mf.pictures[0].data
        tags = mf.tags
        if tags is None:
            return None
        if hasattr(tags, "getall"):                       # ID3 (MP3)
            apics = tags.getall("APIC")
            if apics:
                return apics[0].data
        covr = tags.get("covr")                           # M4A
        if covr:
            return bytes(covr[0])
    except Exception:  # noqa: BLE001
        pass
    return None


def set_cover(path, data):
    """Cover vervangen (data=bytes) of verwijderen (data=None).
    Werkt voor MP3/FLAC/M4A; geeft fouttekst of None."""
    try:
        mf = mutagen.File(path)
        if mf is None or mf.tags is None:
            return "geen ondersteunde tags"
        if hasattr(mf, "pictures"):                       # FLAC
            mf.clear_pictures()
            if data:
                pic = _FlacPicture()
                pic.type = 3
                pic.mime = "image/jpeg"
                pic.data = data
                mf.add_picture(pic)
            mf.save()
            return None
        tags = mf.tags
        if hasattr(tags, "delall"):                       # ID3 (MP3)
            from mutagen.id3 import APIC
            tags.delall("APIC")
            if data:
                tags.add(APIC(encoding=3, mime="image/jpeg", type=3,
                              desc="Cover", data=data))
            mf.save()
            return None
        if "covr" in tags or hasattr(tags, "get"):        # M4A
            from mutagen.mp4 import MP4Cover
            if data:
                tags["covr"] = [MP4Cover(data)]
            else:
                del tags["covr"]
            mf.save()
            return None
        return "cover niet ondersteund in dit formaat"
    except Exception as e:  # noqa: BLE001
        return str(e)


# === Converteren: velden en helpers ===

# De velden die Lopus kent (bewust kort: wat je echt gebruikt).
# (placeholder, interne tag-key of None, uitleg)
CONV_FIELDS = [
    ("%title%", "title", "Titel van het nummer"),
    ("%artist%", "artist", "Artiest"),
    ("%albumartist%", "albumartist", "Albumartiest"),
    ("%album%", "album", "Album"),
    ("%track%", "track", "Tracknummer (bijv. 3)"),
    ("%discnumber%", "disc", "Schijfnummer (bijv. 1)"),
    ("%year%", "year", "Jaar (bijv. 1987)"),
    ("%genre%", "genre", "Genre"),
    ("%comment%", "comment", "Commentaar"),
    ("%composer%", "composer", "Componist"),
    ("%dummy%", None, "Dit deel overslaan (niet importeren)"),
]


def _row_tags(row):
    """Effectieve tagwaarden van een rij: orig + niet-opgeslagen wijzigen."""
    v = dict(row.get("orig", {}))
    v.update(row.get("changed", {}))
    return v


def _fmt_substitute(fmt, tags):
    """Vul %velden% in met tagwaarden; onbekend wordt leeg."""
    out = fmt
    for ph, key, _h in CONV_FIELDS:
        if ph in out:
            out = out.replace(ph, str(tags.get(key, "") or ""))
    return out


def _sanitize_filename(name):
    for ch in '/\\:*?"<>|':
        name = name.replace(ch, "_")
    return name.strip() or "naamloos"


def _clear_fg(widget, item):
    """Zet de voorgrond terug op de standaard venster-tekstkleur.
    (Een lege QColor of null-QBrush tekent zwart in donkere thema's.)"""
    item.setForeground(widget.palette().color(QPalette.ColorRole.WindowText))


def _fmt_to_regex(fmt, allow_ext=False):
    """Zet een format-string om in een regex met benoemde groepen.
    allow_ext: staat optioneel een bestandsextensie toe aan het eind
    (voor parsen van de volledige bestandsnaam)."""
    import re
    keys = {ph[1:-1]: key for ph, key, _h in CONV_FIELDS}
    pat = ""
    for chunk in re.split("(%[a-z]+%)", fmt):
        if chunk.startswith("%") and chunk.endswith("%") and chunk[1:-1] in keys:
            pat += f"(?P<{chunk[1:-1]}>.*?)"
        else:
            pat += re.escape(chunk)
    if allow_ext:
        pat += r"(?:\.[A-Za-z0-9]{1,5})?"
    return re.compile("^" + pat + "$")


def _push_fmt_history(mode, fmt):
    cfg = _load_cfg()
    hist = [f for f in cfg.get("fmt_" + mode, []) if f != fmt]
    hist.insert(0, fmt)
    cfg["fmt_" + mode] = hist[:8]
    _save_cfg(cfg)


class FormatDialog(QDialog):
    """Formaat-dialoog voor Converteren: invoer + veld-lijst + geschiedenis
    + live voorbeeld."""

    def __init__(self, mode, rows, parent=None):
        super().__init__(parent)
        from PyQt6.QtWidgets import QComboBox, QToolButton
        self.mode = mode
        self.rows = rows
        self.format_string = ""
        titel = tr({"tag2fn": "Tag - Bestandsnaam",
                 "fn2tag": "Bestandsnaam - Tag",
                 "tag2tag": "Tag - Tag"}.get(mode, tr("Converteren")))
        self.setWindowTitle(titel)
        self.resize(560, 250)
        lay = QVBoxLayout(self)
        first = os.path.basename(rows[0]["path"]) if rows else ""
        lay.addWidget(QLabel(f"Formattering: {first}"))
        self.combo = QComboBox()
        self.combo.setEditable(True)
        hist = _load_cfg().get("fmt_" + mode, [])
        if not hist:
            hist = {"tag2fn": ["%artist% - %track% - %title%",
                               "%track% - %title%"],
                    "fn2tag": ["%artist% - %track% - %title%",
                               "%track% - %title%"],
                    "tag2tag": ["%artist% - %album%"]}.get(mode, [])
        self.combo.addItems(hist)
        self.combo.setCurrentText(hist[0] if hist else "")
        lay.addWidget(self.combo)
        self.combo.editTextChanged.connect(lambda _t: self._update_preview())

        # Veld-keuze: pijl-knop met uitleg per veld
        row = QHBoxLayout()
        row.addWidget(self.combo, 1)
        fld_btn = QToolButton()
        fld_btn.setText("▶")
        fld_btn.setToolTip(tr("Veld invoegen"))
        fld_btn.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        m = QMenu(fld_btn)
        for ph, _key, uitleg in CONV_FIELDS:
            a = m.addAction(f"{ph}   — {uitleg}")
            a.triggered.connect(lambda _c=False, p=ph: self._insert_field(p))
        fld_btn.setMenu(m)
        row.addWidget(fld_btn)
        lay.addLayout(row)

        # Doelveld (alleen Tag - Tag)
        self.target_combo = None
        if mode == "tag2tag":
            lay.addWidget(QLabel(tr("Doelveld (krijgt de nieuwe waarde):")))
            self.target_combo = QComboBox()
            for ph, key, uitleg in CONV_FIELDS:
                if key is not None:
                    self.target_combo.addItem(f"{ph}   — {uitleg}", key)
            self.target_combo.setCurrentIndex(
                max(0, self.target_combo.findData("comment")))
            lay.addWidget(self.target_combo)
            self.target_combo.currentIndexChanged.connect(
                lambda _i: self._update_preview())

        self.preview = QLabel("")
        self.preview.setWordWrap(True)
        self.preview.setStyleSheet("font-family: monospace;")
        lay.addWidget(self.preview)
        lay.addStretch(1)

        brow = QHBoxLayout()
        btn_ok = QPushButton("OK")
        btn_cancel = QPushButton(tr("Annuleren"))
        btn_cancel.clicked.connect(self.reject)
        btn_ok.clicked.connect(self.accept)
        btn_ok.setDefault(True)
        brow.addStretch(1)
        brow.addWidget(btn_ok)
        brow.addWidget(btn_cancel)
        lay.addLayout(brow)
        self._update_preview()

    def _insert_field(self, ph):
        self.combo.setFocus()
        self.combo.lineEdit().insert(ph)

    def _update_preview(self):
        if not self.rows:
            self.preview.setText("")
            return
        fmt = self.combo.currentText()
        lines = []
        if self.mode == "tag2fn":
            r0 = self.rows[0]
            new = _sanitize_filename(_fmt_substitute(fmt, _row_tags(r0)))
            lines.append(new + os.path.splitext(r0["path"])[1])
        elif self.mode == "fn2tag":
            for r0 in self.rows[:3]:
                base = os.path.splitext(os.path.basename(r0["path"]))[0]
                mm = _fmt_to_regex(fmt).match(base)
                if mm:
                    d = {k: v for k, v in mm.groupdict().items()
                         if k != "dummy"}
                    lines.append(base + "  →  " + ", ".join(
                        f"{k}={v}" for k, v in d.items()))
                else:
                    lines.append(base + "  →  ⚠ komt niet overeen")
        else:  # tag2tag
            lines.append(_fmt_substitute(fmt, _row_tags(self.rows[0])))
        self.preview.setText("\n".join(lines))

    def accept(self):
        self.format_string = self.combo.currentText().strip()
        if not self.format_string:
            return
        _push_fmt_history(self.mode, self.format_string)
        super().accept()


class LopusFilePicker(QDialog):
    """Lichte bestands-/mapkeuze in Lopus-stijl: begint in de home-map,
    geen externe bestandsbeheerder."""

    def __init__(self, parent=None, mode="dir", exts=None,
                 start=None, title="Kiezen"):
        super().__init__(parent)
        self.mode = mode            # "dir" of "open"
        self.exts = [e.lower() for e in (exts or [])]
        self.selected = ""
        self.setWindowTitle(title)
        self.resize(560, 480)
        lay = QVBoxLayout(self)
        top = QHBoxLayout()
        top.addWidget(QLabel(tr("Map:")))
        self.path_edit = QLineEdit()
        self.path_edit.returnPressed.connect(self._reload)
        top.addWidget(self.path_edit, 1)
        up = QPushButton("⬆")
        up.setFixedWidth(34)
        up.clicked.connect(self._go_up)
        top.addWidget(up)
        lay.addLayout(top)
        from PyQt6.QtWidgets import QListWidget
        self.listw = QListWidget()
        self.listw.itemDoubleClicked.connect(self._dbl)
        lay.addWidget(self.listw, 1)
        brow = QHBoxLayout()
        ok = QPushButton(tr("Kiezen"))
        cancel = QPushButton(tr("Annuleren"))
        cancel.clicked.connect(self.reject)
        ok.clicked.connect(self._choose)
        ok.setDefault(True)
        brow.addStretch(1)
        brow.addWidget(ok)
        brow.addWidget(cancel)
        lay.addLayout(brow)
        self._cur = start or os.path.expanduser("~")
        self._reload()

    def _go_up(self):
        p = os.path.dirname(self._cur.rstrip("/"))
        if p and p != self._cur:
            self._cur = p
            self._reload()

    def _reload(self):
        self._cur = os.path.abspath(os.path.expanduser(
            self.path_edit.text().strip() or self._cur))
        if not os.path.isdir(self._cur):
            self._cur = os.path.expanduser("~")
        self.path_edit.setText(self._cur)
        self.selected = ""
        self.listw.clear()
        try:
            namen = sorted(os.listdir(self._cur), key=str.lower)
        except OSError:
            return
        for naam in namen:
            if naam.startswith("."):
                continue
            vol = os.path.join(self._cur, naam)
            if os.path.isdir(vol):
                from PyQt6.QtWidgets import QListWidgetItem
                it = QListWidgetItem("📁 " + naam)
                it.setData(Qt.ItemDataRole.UserRole, ("dir", vol))
                self.listw.addItem(it)
        if self.mode == "open":
            for naam in namen:
                vol = os.path.join(self._cur, naam)
                if (os.path.isfile(vol)
                        and (not self.exts or os.path.splitext(naam)[1].lower()
                             in self.exts)):
                    from PyQt6.QtWidgets import QListWidgetItem
                    it = QListWidgetItem("🎵 " + naam
                                         if os.path.splitext(naam)[1].lower()
                                         in (".mp3", ".flac", ".ogg", ".m4a",
                                             ".wma")
                                         else "📄 " + naam)
                    it.setData(Qt.ItemDataRole.UserRole, ("file", vol))
                    self.listw.addItem(it)

    def _dbl(self, item):
        soort, vol = item.data(Qt.ItemDataRole.UserRole)
        if soort == "dir":
            self._cur = vol
            self.path_edit.setText(vol)
            self._reload()
        elif self.mode == "open":
            self.selected = vol
            self.accept()

    def _choose(self):
        sel = self.listw.currentItem()
        if sel is not None:
            soort, vol = sel.data(Qt.ItemDataRole.UserRole)
            if self.mode == "dir" and soort == "dir":
                self.selected = vol
                self.accept()
                return
            if self.mode == "open" and soort == "file":
                self.selected = vol
                self.accept()
                return
        if self.mode == "dir":
            self.selected = self._cur
            self.accept()

# === DEEL 1B ===

def _read_info(path):
    """Alle kolomwaarden van één bestand lezen."""
    row = {"path": path, "isdir": False, "orig": {}, "info": {}}
    st = os.stat(path)
    row["info"]["file"] = os.path.basename(path)
    row["info"]["map"] = os.path.dirname(path)
    row["info"]["size"] = st.st_size
    row["info"]["modified"] = time.strftime(
        "%d-%m-%Y %H:%M", time.localtime(st.st_mtime))
    cover = get_cover(path)
    row["info"]["cover"] = "ja" if cover else ""
    row["info"]["_coverdata"] = cover
    try:
        mf = mutagen.File(path, easy=True)
        t = (mf.tags or {}) if mf else {}

        def g(key):
            v = t.get(key) or []
            return str(v[0]).strip() if v else ""

        for key in ("title", "artist", "album", "albumartist", "genre",
                    "comment", "composer"):
            row["orig"][key] = g(key)
        for src, dst in (("tracknumber", "track"), ("discnumber", "disc")):
            row["orig"][dst] = g(src).split("/")[0]
        row["orig"]["year"] = g("date")[:10]
        row["orig"]["bpm"] = g("bpm")
        info = mf.info
        row["info"]["codec"] = os.path.splitext(path)[1].lstrip(".").upper()
        row["info"]["bitrate"] = (
            f"{getattr(info, 'bitrate', 0) // 1000} kb/s"
            if getattr(info, "bitrate", 0) else "")
        row["info"]["samplerate"] = (
            f"{info.sample_rate} Hz" if getattr(info, "sample_rate", 0) else "")
        row["info"]["length"] = _fmt_len(getattr(info, "length", 0))
    except Exception:  # noqa: BLE001 - leesfout: lege tags
        pass
    return row


class TagDelegate(QStyledItemDelegate):
    """Delegate met pijltoetsen in de editor: Omlaag/Omhoog slaat de cel
    op en opent meteen de cel eronder/erboven (snel tracknummers werken)."""

    def __init__(self, table):
        super().__init__(table)
        self._table = table

    def createEditor(self, parent, option, index):
        ed = super().createEditor(parent, option, index)
        ed.installEventFilter(self)
        return ed

    def eventFilter(self, obj, ev):
        if (ev.type() == ev.Type.KeyPress
                and isinstance(obj, QLineEdit)):
            key = ev.key()
            if key in (Qt.Key.Key_Down, Qt.Key.Key_Up,
                       Qt.Key.Key_Return, Qt.Key.Key_Enter):
                delta = (-1 if key == Qt.Key.Key_Up else 1)
                self.commitData.emit(obj)
                self.closeEditor.emit(
                    obj, QStyledItemDelegate.EndEditHint.SubmitModelCache)
                t = self._table
                r, c = t.currentRow(), t.currentColumn()
                nr = r + delta

                def go():
                    if 0 <= nr < t.rowCount():
                        it = t.item(nr, c)
                        if (it is not None
                                and it.data(Qt.ItemDataRole.UserRole + 2) is None):
                            t.setCurrentCell(nr, c)
                            t.editItem(it)

                QTimer.singleShot(0, go)
                return True
        return super().eventFilter(obj, ev)


class CellItem(QTableWidgetItem):
    """Tabelitem met slimme sortering: lege velden bovenaan, numerieke
    kolommen (Nr, Jaar, Grootte, ...) op waarde i.p.v. alfabetisch."""

    def __lt__(self, other):
        a, b = self.text().strip(), other.text().strip()

        def num(s):
            if not s:
                return None
            try:
                return float(s.replace(",", "."))
            except ValueError:
                return None

        na, nb = num(a), num(b)
        if na is not None and nb is not None:
            return na < nb
        if not a and b:
            return True          # leeg eerst (bovenaan bij oplopend)
        if a and not b:
            return False
        if not a and not b:
            return False
        return a.lower() < b.lower()


class DirScanWorker(QThread):
    """Leest mappen (optioneel met submappen) op de achtergrond en
    stuurt rijen in batches — ook 100.000+ bestanden blijven responsief."""
    rows_ready = pyqtSignal(list)     # lijst van row-dicts
    dir_found = pyqtSignal(str)       # submap (voor navigatie-rijen)
    done = pyqtSignal(int)            # totaal aantal bestanden (-1=gestopt)

    def __init__(self, folder, subdirs=False, parent=None):
        super().__init__(parent)
        self.folder = folder
        self.subdirs = subdirs
        self.stop_requested = False

    def stop(self):
        self.stop_requested = True

    def run(self):
        count = 0
        batch = []

        def walk(top):
            nonlocal count, batch
            try:
                entries = sorted(os.scandir(top),
                                 key=lambda e: e.name.lower())
            except OSError:
                return
            for e in entries:
                if self.stop_requested:
                    return
                if e.is_dir(follow_symlinks=False):
                    self.dir_found.emit(e.path)
                    if self.subdirs:
                        walk(e.path)
                        if self.stop_requested:
                            return
                    continue
                ext = os.path.splitext(e.name)[1].lower()
                if ext not in AUDIO_EXTS:
                    continue
                try:
                    batch.append(_read_info(e.path))
                    count += 1
                except OSError:
                    continue
                if len(batch) >= 150:
                    self.rows_ready.emit(list(batch))
                    batch = []

        walk(self.folder)
        if batch and not self.stop_requested:
            self.rows_ready.emit(batch)
        self.done.emit(-1 if self.stop_requested else count)

# === DEEL 2: de TagEditor-klasse ===

class TagEditor(QDialog):
    """Mp3tag-achtige tag-editor: raster met bestanden en tags, linkerpaneel
    met bulk-velden voor de selectie en een cover-editor."""

    def __init__(self, main=None):
        super().__init__(main)
        self.setWindowFlag(Qt.WindowType.Window)
        self.setWindowTitle("🎵 Tag-editor")
        self.resize(1280, 700)
        self.main = main
        self.rows = []            # row-dicts van bestanden
        self.dir_rows = []        # submappen (navigatie)
        self._loading = False
        self._worker = None
        self._row_index = {}      # tabelrij -> index in self.rows
        self._dir_index = {}      # tabelrij -> submappad
        self._table_row_of = {}   # rowidx -> tabelrij (na sorteren)
        cfg = _load_cfg()
        self.visible_cols = [k for k, _t, _e, v in COLUMNS
                             if v and cfg.get("col_" + k, True)]

        lay = QVBoxLayout(self)
        # Bovenbalk: pad + navigatie + kolommen + submappen
        top = QHBoxLayout()
        self.path_edit = QLineEdit()
        self.path_edit.setPlaceholderText(tr("Map met muziek..."))
        self.path_edit.returnPressed.connect(
            lambda: self.load_folder(self.path_edit.text()))
        top.addWidget(QLabel(tr("Map:")), 0)
        top.addWidget(self.path_edit, 1)
        btn_up = QPushButton("⬆")
        btn_up.setToolTip(tr("Naar de bovenliggende map"))
        btn_up.setFixedWidth(34)
        btn_up.clicked.connect(self.go_up)
        btn_down = QPushButton("⬇")
        btn_down.setToolTip(tr("De geselecteerde map-rij in (dubbelklik kan ook)"))
        btn_down.setFixedWidth(34)
        btn_down.clicked.connect(self.go_down)
        btn_browse = QPushButton(tr("📂 Bladeren..."))
        btn_browse.setToolTip(tr("Map kiezen met de Lopus-bestandsbeheerder"))
        btn_browse.clicked.connect(self.browse_folder)
        btn_add = QPushButton(tr("➕ Map toevoegen"))
        btn_add.setToolTip(tr("Bestanden van een extra map onderaan bijvoegen"))
        btn_add.clicked.connect(self.add_folder)
        btn_reload = QPushButton("🔄")
        btn_reload.setToolTip(tr("Herladen"))
        btn_reload.setFixedWidth(34)
        btn_reload.clicked.connect(
            lambda: self.load_folder(self.path_edit.text()))
        self.subdirs_chk = QPushButton("📁+")
        self.subdirs_chk.setToolTip(
            tr("Submappen toevoegen aan de lijst (aan/uit)"))
        self.subdirs_chk.setCheckable(True)
        self.subdirs_chk.setFixedWidth(44)
        self.subdirs_chk.toggled.connect(
            lambda _c: self.load_folder(self.path_edit.text()))
        self.cols_btn = QToolButton()
        self.cols_btn.setText(tr("👁 Kolommen"))
        self.cols_btn.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        self._build_col_menu()
        top.addWidget(btn_up)
        top.addWidget(btn_down)
        top.addWidget(btn_browse)
        top.addWidget(btn_add)
        top.addWidget(btn_reload)
        top.addWidget(self.subdirs_chk)
        top.addWidget(self.cols_btn)
        lay.addLayout(top)
        # Menubalk: Bestand / Beeld / Converteren
        mb = QMenuBar(self)
        self._build_menubar(mb)
        lay.setMenuBar(mb)

        # Splitter: links bulk+cover, rechts tabel
        split = QSplitter(Qt.Orientation.Horizontal)
        lay.addWidget(split, 1)

        left = QWidget()
        llay = QVBoxLayout(left)
        llay.setContentsMargins(0, 0, 0, 0)
        self.bulk_edits = {}
        for key, label, kind in BULK_FIELDS:
            if kind == "small":
                roww = QWidget()
                h = QHBoxLayout(roww)
                h.setContentsMargins(0, 0, 0, 0)
                h.addWidget(QLabel(label), 0)
                ed = QLineEdit()
                ed.returnPressed.connect(
                    lambda k=key, e=ed: self._apply_bulk(k, e))
                h.addWidget(ed, 1)
                llay.addWidget(roww)
            else:
                llay.addWidget(QLabel(label), 0)
                ed = QLineEdit()
                ed.returnPressed.connect(
                    lambda k=key, e=ed: self._apply_bulk(k, e))
                llay.addWidget(ed)
            self.bulk_edits[key] = ed
        btn_num = QPushButton(tr("🔢 Nummeren 1..N (selectie)"))
        btn_num.setToolTip(tr("Geselecteerde rijen opeenvolgend nummeren"))
        btn_num.clicked.connect(self._renumber)
        llay.addWidget(btn_num)
        llay.addWidget(QLabel(tr("Cover (selectie):")), 0)
        self.cover_lbl = QLabel()
        self.cover_lbl.setFixedSize(160, 160)
        self.cover_lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.cover_lbl.setStyleSheet(
            "border: 1px solid palette(mid); background: palette(base);")
        llay.addWidget(self.cover_lbl)
        crow = QHBoxLayout()
        btn_cov_load = QPushButton(tr("📷 Laden..."))
        btn_cov_load.setToolTip(
            tr("Afbeelding als cover instellen voor de selectie"))
        btn_cov_load.clicked.connect(self._cover_load)
        btn_cov_del = QPushButton(tr("🗑 Weg"))
        btn_cov_del.setToolTip(tr("Cover van de selectie verwijderen"))
        btn_cov_del.clicked.connect(self._cover_remove)
        crow.addWidget(btn_cov_load)
        crow.addWidget(btn_cov_del)
        crow.addStretch(1)
        llay.addLayout(crow)
        llay.addStretch(1)
        split.addWidget(left)

        right = QWidget()
        rlay = QVBoxLayout(right)
        rlay.setContentsMargins(0, 0, 0, 0)
        self.table = QTableWidget(0, len(COLUMNS))
        self.table.setHorizontalHeaderLabels([t for _k, t, _e, _v in COLUMNS])
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setSelectionBehavior(
            QAbstractItemView.SelectionBehavior.SelectRows)
        self.table.setSelectionMode(
            QAbstractItemView.SelectionMode.ExtendedSelection)
        # Bewerken: één klik (via cellClicked), F2 of direct typen.
        # Dubbelklik bewust géén edit-trigger: die opent de muziekspeler.
        self.table.setEditTriggers(
            QAbstractItemView.EditTrigger.EditKeyPressed
            | QAbstractItemView.EditTrigger.AnyKeyPressed)
        self.table.setSortingEnabled(True)   # lege velden komen bovenaan
        self.table.horizontalHeader().setSortIndicator(
            -1, Qt.SortOrder.AscendingOrder)
        # Kolommen versleepbaar via de kop; volgorde wordt onthouden
        hdr = self.table.horizontalHeader()
        hdr.setSectionsMovable(True)
        hdr.setSectionsClickable(True)
        self._restore_col_order()
        hdr.sectionMoved.connect(lambda *_a: self._save_col_order())
        self.table.itemChanged.connect(self._on_item_changed)
        self.table.itemDoubleClicked.connect(self._on_double_click)
        self.table.cellClicked.connect(self._on_cell_clicked)
        self.table.itemSelectionChanged.connect(self._on_selection_changed)
        # Rechtermuisknop-menu in de tabel
        self.table.setContextMenuPolicy(
            Qt.ContextMenuPolicy.CustomContextMenu)
        self.table.customContextMenuRequested.connect(self._table_menu)
        # Pijltjes/Enter in de editor: cel opslaan + naar rij eronder/erboven
        self._delegate = TagDelegate(self.table)
        self.table.setItemDelegate(self._delegate)
        # Als de celleditor dichtgaat (Enter, Esc, focusverlies): de rij
        # netjes navertellen, anders kunnen cellen vals-rood blijven staan
        # terwijl de buffer al is opgeslagen.
        self._delegate.closeEditor.connect(self._on_editor_closed)
        rlay.addWidget(self.table, 1)
        self.status_lbl = QLabel("")
        rlay.addWidget(self.status_lbl)
        split.addWidget(right)
        split.setSizes([280, 1000])

        self._apply_column_visibility()
        self._set_cover_preview(None)

        # In een QDialog is elke QPushButton impliciet autoDefault: Enter
        # (bijv. in een bulkveld) klikte dan stilletjes de ⬆-knop erbij,
        # waardoor de editor naar de bovenliggende map sprong. Alles uit.
        for b in self.findChildren(QPushButton):
            b.setAutoDefault(False)
            b.setDefault(False)

    def keyPressEvent(self, ev):
        # Enter/Return op dialoog-niveau nooit doorlaten (geen default-knop,
        # geen accept()); alleen Esc mag de dialoog sluiten.
        if ev.key() in (Qt.Key.Key_Return, Qt.Key.Key_Enter):
            ev.accept()
            return
        super().keyPressEvent(ev)

    # ---------- menubalk ----------
    def _build_menubar(self, mb):
        m_file = mb.addMenu(tr("&Bestand"))
        act_browse = QAction(tr("Map openen...  📂"), self)
        act_browse.triggered.connect(self.browse_folder)
        act_add = QAction(tr("Map toevoegen...  ➕"), self)
        act_add.triggered.connect(self.add_folder)
        act_reload = QAction(tr("Herladen  🔄"), self)
        act_reload.triggered.connect(
            lambda: self.load_folder(self.path_edit.text()))
        act_save = QAction(tr("Alles opslaan  💾"), self)
        act_save.triggered.connect(self.save_all)
        act_revert = QAction(tr("Terugdraaien  ↩️"), self)
        act_revert.triggered.connect(self.revert)
        act_close = QAction(tr("Sluiten"), self)
        act_close.triggered.connect(self.close)
        for a in (act_browse, act_add, act_reload):
            m_file.addAction(a)
        m_file.addSeparator()
        for a in (act_save, act_revert):
            m_file.addAction(a)
        m_file.addSeparator()
        m_file.addAction(act_close)

        m_view = mb.addMenu("&Beeld")
        m_cols = m_view.addMenu(tr("Kolommen  👁"))
        self._fill_col_menu(m_cols)
        act_sub = QAction(tr("Submappen inladen  📁+"), self)
        act_sub.setCheckable(True)
        act_sub.setChecked(self.subdirs_chk.isChecked())
        act_sub.toggled.connect(self.subdirs_chk.setChecked)
        m_view.addAction(act_sub)

        m_conv = mb.addMenu(tr("&Converteren"))
        act_tf = QAction(tr("Tag - Bestandsnaam..."), self)
        act_tf.triggered.connect(self._conv_tag2fn)
        act_ft = QAction(tr("Bestandsnaam - Tag..."), self)
        act_ft.triggered.connect(self._conv_fn2tag)
        act_tt = QAction(tr("Tag - Tag..."), self)
        act_tt.triggered.connect(self._conv_tag2tag)
        for a in (act_tf, act_ft, act_tt):
            m_conv.addAction(a)
        m_conv.addSeparator()
        act_num = QAction(tr("Wizard Autonummering..."), self)
        act_num.triggered.connect(self._wizard_autonum)
        m_conv.addAction(act_num)
        act_case = QAction(tr("Hoofdletters normaliseren..."), self)
        act_case.triggered.connect(self._conv_case)
        m_conv.addAction(act_case)
        act_lookup = QAction(tr("🌐 Online opzoeken (Discogs / MusicBrainz)..."),
                             self)
        act_lookup.triggered.connect(self._lookup_online)
        m_online = mb.addMenu(tr("🌐 &Online"))
        m_online.addAction(act_lookup)

    def _fill_col_menu(self, menu):
        for key, title, editable, default in COLUMNS:
            if not editable:
                title += "  (info)"
            act = menu.addAction(title)
            act.setCheckable(True)
            act.setChecked(key in self.visible_cols)
            act.toggled.connect(lambda on, k=key: self._toggle_column(k, on))
        menu.addSeparator()
        act_reset = menu.addAction(tr("Standaard"))
        act_reset.triggered.connect(self._reset_columns)

    def _build_col_menu(self):
        menu = QMenu(self)
        for key, title, editable, default in COLUMNS:
            if not editable:
                title += "  (info)"
            act = menu.addAction(title)
            act.setCheckable(True)
            act.setChecked(key in self.visible_cols)
            act.toggled.connect(lambda on, k=key: self._toggle_column(k, on))
        menu.addSeparator()
        act_reset = menu.addAction(tr("Standaard"))
        act_reset.triggered.connect(self._reset_columns)
        self.cols_btn.setMenu(menu)

    def _toggle_column(self, key, on):
        if on and key not in self.visible_cols:
            self.visible_cols.append(key)
        elif not on and key in self.visible_cols:
            self.visible_cols.remove(key)
        cfg = _load_cfg()
        cfg["col_" + key] = on
        _save_cfg(cfg)
        self._apply_column_visibility()

    def _reset_columns(self):
        self.visible_cols = [k for k, _t, _e, v in COLUMNS if v]
        cfg = _load_cfg()
        for k, _t, _e, v in COLUMNS:
            cfg["col_" + k] = v
        cfg.pop("col_order", None)
        _save_cfg(cfg)
        self._apply_column_visibility()

    # ---------- kolomvolgorde onthouden ----------
    def _restore_col_order(self):
        order = _load_cfg().get("col_order")
        if not order:
            return
        log_of_key = {k: c for c, (k, _t, _e, _v) in enumerate(COLUMNS)}
        hdr = self.table.horizontalHeader()
        for vis, key in enumerate(order):
            log = log_of_key.get(key)
            if log is None:
                continue
            cur = hdr.visualIndex(log)
            if cur != vis:
                hdr.moveSection(cur, vis)

    def _save_col_order(self):
        hdr = self.table.horizontalHeader()
        order = []
        for v in range(len(COLUMNS)):
            log = hdr.logicalIndex(v)
            if 0 <= log < len(COLUMNS):
                order.append(COLUMNS[log][0])
        cfg = _load_cfg()
        cfg["col_order"] = order
        _save_cfg(cfg)

    def _apply_column_visibility(self):
        for c, (key, _t, _e, _v) in enumerate(COLUMNS):
            self.table.setColumnHidden(c, key not in self.visible_cols)

# === DEEL 2B2: laden / vullen ===

    # ---------- laden ----------
    def load_folder(self, folder, append=False):
        if not isinstance(folder, str):
            self.status_lbl.setText(tr("⚠ Geen geldige map."))
            return
        folder = os.path.abspath(os.path.expanduser(folder.strip()))
        if not folder or not os.path.isdir(folder):
            self.status_lbl.setText(f"⚠ Geen geldige map: {folder}")
            return
        if mutagen is None:
            self.status_lbl.setText(
                "⚠ mutagen ontbreekt: pip install --user mutagen")
            return
        # Eerder scanwerk stoppen én loskoppelen: anders komen er nog
        # nabezig-batches van de OUDE map in de nieuwe weergave terecht.
        if self._worker is not None:
            try:
                self._worker.rows_ready.disconnect(self._add_rows)
                self._worker.dir_found.disconnect(self._add_dir_row)
                self._worker.done.disconnect(self._scan_done)
            except (TypeError, RuntimeError):
                pass
            self._worker.stop()
        self.table.setSortingEnabled(False)  # tijdens vullen niet sorteren
        if not append:
            self._loading = True
            try:
                self.rows = []
                self.dir_rows = []
                self._row_index = {}
                self._dir_index = {}
                self._table_row_of = {}
                self.table.setRowCount(0)
            finally:
                self._loading = False
            self.folders = [folder]
        else:
            self.folders = getattr(self, "folders", []) + [folder]
        self.path_edit.setText(folder)
        self._worker = DirScanWorker(folder,
                                     self.subdirs_chk.isChecked(), self)
        self._worker.rows_ready.connect(self._add_rows)
        self._worker.dir_found.connect(self._add_dir_row)
        self._worker.done.connect(self._scan_done)
        self._worker.start()
        self.status_lbl.setText(f"⏳ Map lezen: {folder} ...")

    def _add_dir_row(self, path):
        if self.subdirs_chk.isChecked():
            return  # bij submap-scan hoeven we niet te navigeren
        self._loading = True
        try:
            r = self.table.rowCount()
            self.table.insertRow(r)
            it = QTableWidgetItem("📁 " + os.path.basename(path) + "/")
            it.setFlags(Qt.ItemFlag.ItemIsEnabled
                        | Qt.ItemFlag.ItemIsSelectable)
            it.setForeground(QColor("#7aa7ff"))
            it.setData(Qt.ItemDataRole.UserRole + 2, path)
            self.table.setItem(r, 0, it)
            self._dir_index[r] = path
            self.dir_rows.append(path)
        finally:
            self._loading = False

    def _add_rows(self, batch):
        self._loading = True
        try:
            start = self.table.rowCount()
            self.table.setRowCount(start + len(batch))
            for i, row in enumerate(batch):
                self.rows.append(row)
                self._fill_row(start + i, len(self.rows) - 1, row)
        finally:
            self._loading = False

    def _scan_done(self, total):
        self._rebuild_row_map()
        self.table.horizontalHeader().setSortIndicator(
            -1, Qt.SortOrder.AscendingOrder)
        self.table.setSortingEnabled(True)   # sorteren weer aan
        if total < 0:
            self.status_lbl.setText(tr("⏹ Gestopt."))
        else:
            self.status_lbl.setText(
                tr("{n} bestand(en) geladen uit {pad}").format(
                    n=total, pad=self.path_edit.text())
                + (tr("  (incl. submappen)") if self.subdirs_chk.isChecked()
                   else ""))

    def _col_value(self, row, key):
        """Celwaarde: bewerkbare tag of info-kolom."""
        if key in row["orig"]:
            return row["orig"][key]
        v = row["info"].get(key, "")
        if key == "size":
            try:
                from lopus import human_size
                v = human_size(v)
            except Exception:  # noqa: BLE001
                pass
        return str(v)

    def _fill_row(self, r, rowidx, row):
        for c, (key, _t, editable, _v) in enumerate(COLUMNS):
            val = self._col_value(row, key)
            txt = ("🖼 " + val) if (key == "cover" and val) else str(val)
            it = CellItem(txt)
            if not editable:
                it.setFlags(it.flags() & ~Qt.ItemFlag.ItemIsEditable)
                it.setForeground(QColor("#8a8a8a"))
            it.setData(Qt.ItemDataRole.UserRole, rowidx)
            self.table.setItem(r, c, it)
        self.table.setRowHeight(r, 20)
        self._table_row_of[rowidx] = r

    def _rebuild_row_map(self):
        """Mapping rowidx -> tabelrij (nodig na het sorteren)."""
        self._table_row_of = {}
        for r in range(self.table.rowCount()):
            it = self.table.item(r, 0)
            if it is None:
                continue
            idx = it.data(Qt.ItemDataRole.UserRole)
            if idx is not None:
                self._table_row_of[idx] = r

# === DEEL 2B3: navigatie / selectie ===

    def go_up(self):
        cur = self.path_edit.text().strip()
        parent = os.path.dirname(os.path.abspath(cur)) if cur else ""
        if parent and os.path.isdir(parent) and parent != cur:
            self.load_folder(parent)

    def go_down(self):
        """De geselecteerde map-rij binnen (of de eerste map-rij)."""
        path = None
        for item in self.table.selectedItems():
            p = item.data(Qt.ItemDataRole.UserRole + 2)
            if p:
                path = p
                break
        if not path and self._dir_index:
            path = next(iter(self._dir_index.values()))
        if path:
            self.load_folder(path)
        else:
            self.status_lbl.setText(
                tr("Geen map-rij geselecteerd (dubbelklik kan ook)."))

    def browse_folder(self, append=False):
        """Map kiezen met Lopus' eigen keuzevenster (start in home)."""
        dlg = LopusFilePicker(self, mode="dir", title="Map kiezen")
        if dlg.exec() and dlg.selected and os.path.isdir(dlg.selected):
            self.load_folder(dlg.selected, append=append)

    def add_folder(self):
        self.browse_folder(append=True)

    # ---------- converteren ----------
    def _conv_rows(self):
        """Doelrijen voor conversies: selectie, anders alles."""
        idxs = self._selected_rowidxs()
        self._rebuild_row_map()
        if not idxs:
            idxs = list(range(len(self.rows)))
        return [self.rows[i] for i in idxs if not self.rows[i].get("isdir")]

    def _conv_tag2fn(self):
        rows = self._conv_rows()
        if not rows:
            self.status_lbl.setText(tr("Geen bestanden om te hernoemen."))
            return
        dlg = FormatDialog("tag2fn", rows, self)
        if not dlg.exec():
            return
        fmt = dlg.format_string
        ok = fout = 0
        self._rebuild_row_map()
        for row in rows:
            try:
                d = os.path.dirname(row["path"])
                ext = os.path.splitext(row["path"])[1]
                new = _sanitize_filename(_fmt_substitute(fmt, _row_tags(row)))
                newpath = os.path.join(d, new + ext)
                if newpath == row["path"]:
                    continue
                if os.path.exists(newpath):
                    fout += 1
                    continue
                os.rename(row["path"], newpath)
                row["path"] = newpath
                row["info"]["file"] = new + ext
                row["info"]["map"] = d
                i = next((j for j, r in enumerate(self.rows) if r is row), None)
                if i is not None:
                    trow = self._table_row_of.get(i)
                    if trow is not None:
                        self._fill_row(trow, i, row)
                ok += 1
            except Exception:  # noqa: BLE001
                fout += 1
        self.status_lbl.setText(
            f"🏷 {ok} bestand(en) hernoemd"
            + (f", ⚠ {fout} fout(en)" if fout else "") + ".")

    def _conv_fn2tag(self):
        rows = self._conv_rows()
        if not rows:
            self.status_lbl.setText(tr("Geen bestanden om te importeren."))
            return
        dlg = FormatDialog("fn2tag", rows, self)
        if not dlg.exec():
            return
        fmt = dlg.format_string
        rx = _fmt_to_regex(fmt)
        rx_full = _fmt_to_regex(fmt, allow_ext=True)
        ok = fout = 0
        self._rebuild_row_map()
        self._loading = True
        try:
            for row in rows:
                vol_naam = os.path.basename(row["path"])
                base = os.path.splitext(vol_naam)[0]
                mm = rx.match(base)
                if mm:
                    gebruik = mm
                else:
                    # hele bestandsnaam proberen (met extensie); de
                    # extensie hoort niet in het laatste veld
                    mm = rx_full.match(vol_naam)
                    if not mm:
                        fout += 1
                        continue
                    gebruik = mm
                i = next((j for j, r in enumerate(self.rows) if r is row),
                         None)
                if i is None:
                    fout += 1
                    continue
                gd = gebruik.groupdict()
                # extensie uit het laatste gevulde veld strippen
                gevuld = [k for k, v in gd.items()
                          if k != "dummy" and v]
                if gevuld and gevuld[-1] in gd:
                    laatste = gd[gevuld[-1]]
                    if re.search(r"\.[A-Za-z0-9]{1,5}$", laatste):
                        gd[gevuld[-1]] = os.path.splitext(laatste)[0]
                row.setdefault("changed", {})
                for k, v in gd.items():
                    if k == "dummy" or v is None:
                        continue
                    if v.strip() != row["orig"].get(k, ""):
                        row["changed"][k] = v.strip()
                self._refresh_row_cells(i)
                ok += 1
        finally:
            self._loading = False
        self._update_changed_count()
        self.status_lbl.setText(
            f"🏷 {ok} bestand(en) geïmporteerd"
            + (f", ⚠ {fout} kwamen niet overeen" if fout else "") + ".")
        self._auto_save()

    def _on_cell_clicked(self, r, c):
        """Één klik op een bewerkbare cel = meteen kunnen typen.
        Geen handmatige kleuren: Qt's eigen invoerveld laat zien dat de
        cel actief is (kleur-manipulatie hier veroorzaakte signal-lussen)."""
        if c >= len(COLUMNS) or r >= self.table.rowCount():
            return
        item = self.table.item(r, c)
        if item is None:
            return
        if item.data(Qt.ItemDataRole.UserRole + 2):
            return  # map-rij
        if (COLUMNS[c][0] in EDITABLE_KEYS
                and self.table.currentRow() == r
                and self.table.currentColumn() == c):
            self.table.editItem(item)

    def _on_double_click(self, item):
        """Dubbelklik: map-rij = navigeren; bestand = openen in de
        standaard muziekspeler (zoals Mp3tag doet)."""
        path = item.data(Qt.ItemDataRole.UserRole + 2)
        if path:
            self.load_folder(path)
            return
        idx = item.data(Qt.ItemDataRole.UserRole)
        if idx is not None and idx < len(self.rows):
            subprocess.Popen(["xdg-open", self.rows[idx]["path"]])

    def _edit_next_below(self, item, delta=1):
        """Na Enter/pijltje: door naar dezelfde kolom een rij op/af."""
        r, c = item.row(), item.column()
        if c >= len(COLUMNS) or COLUMNS[c][0] not in EDITABLE_KEYS:
            return
        nr = r + delta
        nxt = self.table.item(nr, c) if 0 <= nr < self.table.rowCount() else None
        if nxt is None:
            return
        if nxt.data(Qt.ItemDataRole.UserRole + 2):
            return  # map-rij: niet bewerken
        self.table.setCurrentCell(nr, c)
        self.table.editItem(nxt)

    # ---------- selectie ----------
    def _selected_rowidxs(self):
        out = []
        for item in self.table.selectedItems():
            idx = item.data(Qt.ItemDataRole.UserRole)
            if idx is not None and idx not in out:
                out.append(idx)
        return out

    def _on_selection_changed(self):
        idxs = self._selected_rowidxs()
        if not idxs:
            self._set_cover_preview(None)
            return
        # Bulk-velden vullen met de gemeenschappelijke waarde (of leeg
        # als de selectie gemengd is) — alleen velden die geen focus hebben.
        for key, _l, _k in BULK_FIELDS:
            vals = {self.rows[i]["orig"].get(key, "") for i in idxs}
            val = vals.pop() if len(vals) == 1 else ""
            ed = self.bulk_edits[key]
            if not ed.hasFocus():
                ed.setText(val)
        cover = self.rows[idxs[0]]["info"].get("_coverdata")
        self._set_cover_preview(cover)

    def _set_cover_preview(self, data):
        if data:
            pm = QPixmap()
            pm.loadFromData(data)
            self.cover_lbl.setPixmap(
                pm.scaled(self.cover_lbl.size(),
                          Qt.AspectRatioMode.KeepAspectRatio,
                          Qt.TransformationMode.SmoothTransformation))
        else:
            self.cover_lbl.setPixmap(QPixmap())
            self.cover_lbl.setText(tr("geen\ncover"))

# === DEEL 2C: bulk / nummeren / cover ===

    # ---------- bulk-bewerking ----------
    def _apply_bulk(self, key, edit):
        """Waarde van het veld toepassen op alle geselecteerde rijen."""
        idxs = self._selected_rowidxs()
        if not idxs:
            self.status_lbl.setText("Eerst rijen selecteren.")
            return
        val = edit.text().strip()
        self._rebuild_row_map()
        self._loading = True
        try:
            for i in idxs:
                row = self.rows[i]
                if val == row["orig"].get(key, ""):
                    continue
                row.setdefault("changed", {})[key] = val
                self._refresh_row_cells(i)
        finally:
            self._loading = False
        edit.setText(val)
        self._update_changed_count()
        self.status_lbl.setText(
            f"{key}: toegepast op {len(idxs)} bestand(en).")
        self._auto_save()

    def _conv_tag2tag(self):
        rows = self._conv_rows()
        if not rows:
            self.status_lbl.setText(tr("Geen bestanden om te wijzigen."))
            return
        dlg = FormatDialog("tag2tag", rows, self)
        if not dlg.exec():
            return
        fmt = dlg.format_string
        target = dlg.target_combo.currentData()
        ok = 0
        self._rebuild_row_map()
        self._loading = True
        try:
            for row in rows:
                val = _fmt_substitute(fmt, _row_tags(row)).strip()
                if not val:
                    continue
                i = next((j for j, r in enumerate(self.rows) if r is row),
                         None)
                if i is None:
                    continue
                if val != row["orig"].get(target, ""):
                    row.setdefault("changed", {})[target] = val
                    self._refresh_row_cells(i)
                ok += 1
        finally:
            self._loading = False
        self._update_changed_count()
        self.status_lbl.setText(f"🏷 {ok} bestand(en) bijgewerkt.")
        self._auto_save()

    def _wizard_autonum(self):
        """Wizard Autonummering: startnummer + aantal cijfers, optioneel
        per map opnieuw beginnen."""
        rows = self._conv_rows()
        if not rows:
            self.status_lbl.setText(tr("Geen bestanden om te nummeren."))
            return
        dlg = QDialog(self)
        dlg.setWindowTitle(tr("Wizard Autonummering"))
        lay = QVBoxLayout(dlg)
        lay.addWidget(QLabel(
            f"Nummert {len(rows)} bestand(en) in lijstvolgorde."))
        h1 = QHBoxLayout()
        h1.addWidget(QLabel(tr("Begin bij nummer:")))
        start = QLineEdit("1")
        start.setFixedWidth(60)
        h1.addWidget(start)
        h1.addStretch(1)
        lay.addLayout(h1)
        h2 = QHBoxLayout()
        h2.addWidget(QLabel(tr("Aantal cijfers (bijv. 2 → 01):")))
        digits = QLineEdit("2")
        digits.setFixedWidth(60)
        h2.addWidget(digits)
        h2.addStretch(1)
        lay.addLayout(h2)
        per_dir = None
        if any(r["info"].get("map") != rows[0]["info"].get("map")
               for r in rows):
            from PyQt6.QtWidgets import QCheckBox
            per_dir = QCheckBox(tr("Per map opnieuw beginnen"))
            lay.addWidget(per_dir)
        brow = QHBoxLayout()
        btn_ok = QPushButton("OK")
        btn_cancel = QPushButton(tr("Annuleren"))
        btn_cancel.clicked.connect(dlg.reject)
        btn_ok.clicked.connect(dlg.accept)
        btn_ok.setDefault(True)
        brow.addStretch(1)
        brow.addWidget(btn_ok)
        brow.addWidget(btn_cancel)
        lay.addLayout(brow)
        if not dlg.exec():
            return
        try:
            n_start = max(1, int(start.text().strip() or "1"))
            n_digits = max(1, min(5, int(digits.text().strip() or "2")))
        except ValueError:
            self.status_lbl.setText("⚠ Ongeldig getal.")
            return
        self._rebuild_row_map()
        self._loading = True
        try:
            n = n_start
            cur_dir = None
            for row in rows:
                if per_dir is not None and per_dir.isChecked():
                    d = row["info"].get("map")
                    if d != cur_dir:
                        cur_dir = d
                        n = n_start
                val = str(n).zfill(n_digits)
                i = next((j for j, r in enumerate(self.rows) if r is row),
                         None)
                if i is None:
                    continue
                if val != row["orig"].get("track", ""):
                    row.setdefault("changed", {})["track"] = val
                    self._refresh_row_cells(i)
                n += 1
        finally:
            self._loading = False
        self._update_changed_count()
        self.status_lbl.setText(f"🔢 {len(rows)} bestand(en) genummerd.")
        self._auto_save()

    def _renumber(self):
        """Geselecteerde rijen opeenvolgend nummeren: 1..N."""
        idxs = sorted(self._selected_rowidxs())
        if not idxs:
            self.status_lbl.setText("Eerst rijen selecteren.")
            return
        self._rebuild_row_map()
        self._loading = True
        try:
            for n, i in enumerate(idxs, start=1):
                row = self.rows[i]
                if str(n) == row["orig"].get("track", ""):
                    continue
                row.setdefault("changed", {})["track"] = str(n)
                self._refresh_row_cells(i)
        finally:
            self._loading = False
        self._update_changed_count()
        self.status_lbl.setText(
            f"🔢 {len(idxs)} bestand(en) genummerd 1..{len(idxs)}.")
        self._auto_save()

    # ---------- hoofdletter-normalisatie ----------
    _SMALL_WORDS = {
        "de", "het", "een", "van", "der", "den", "ter", "te", "tot", "en",
        "of", "in", "op", "voor", "met", "bij", "aan", "uit", "over", "ook",
        "a", "an", "the", "and", "of", "in", "on", "at", "to", "for", "by",
    }

    def _normalize_case(self, text, mode, small_ok):
        """Pas een hoofdletter-stijl toe op één tekstwaarde."""
        s = text.strip()
        if not s:
            return text
        if mode == "lower":
            return s.lower()
        if mode == "upper":
            return s.upper()
        if mode == "sentence":
            # Eerste letter hoofdletter, rest zoals het was (of klein)
            return s[0].upper() + s[1:]
        # title: elk woord met hoofdletter; kleine woorden laag (behalve
        # als eerste woord), als small_ok aan staat
        words = s.split()
        out = []
        for n, w in enumerate(words):
            lw = w.lower()
            if small_ok and n > 0 and lw in self._SMALL_WORDS:
                out.append(lw)
            else:
                out.append(lw[:1].upper() + lw[1:])
        return " ".join(out)

    def _conv_case(self):
        """Hoofdletter-normalisatie over tag-velden van de selectie."""
        rows = self._conv_rows()
        if not rows:
            self.status_lbl.setText(tr("Geen bestanden om te wijzigen."))
            return
        dlg = QDialog(self)
        dlg.setWindowTitle(tr("Hoofdletters normaliseren"))
        lay = QVBoxLayout(dlg)
        lay.addWidget(QLabel(f"Normaliseert {len(rows)} bestand(en)."))

        gb = QGroupBox("Stijl")
        gl = QVBoxLayout(gb)
        radios = {}
        for key, label in (
                ("title", "Elk woord met een hoofdletter (Titelstijl)"),
                ("sentence", "Alleen eerste letter hoofdletter (Zinstijl)"),
                ("upper", "ALLES IN HOOFDLETTERS"),
                ("lower", "alles in kleine letters")):
            rb = QRadioButton(label)
            rb.setChecked(key == "title")
            gl.addWidget(rb)
            radios[key] = rb
        lay.addWidget(gb)

        chk_small = QCheckBox(
            tr("Kleine woordjes (de, van, en, the, ...) laag laten in titelstijl"))
        chk_small.setChecked(True)
        lay.addWidget(chk_small)

        gb2 = QGroupBox(tr("Velden"))
        g2 = QVBoxLayout(gb2)
        field_chks = {}
        for key, lbl in (
                ("file", "Bestandsnaam"),
                ("title", "Titel"), ("artist", "Artiest"),
                ("album", "Album"), ("albumartist", "Albumartiest"),
                ("genre", "Genre"), ("composer", "Componist"),
                ("comment", "Commentaar")):
            cb = QCheckBox(lbl)
            cb.setChecked(key in ("title", "artist"))
            g2.addWidget(cb)
            field_chks[key] = cb
        lay.addWidget(gb2)

        prev = QLabel("")
        prev.setWordWrap(True)
        prev.setStyleSheet("color: gray;")
        lay.addWidget(prev)

        def upd_preview():
            mode = next(k for k, rb in radios.items() if rb.isChecked())
            ex = None
            for row in rows:
                for k, cb in field_chks.items():
                    if cb.isChecked():
                        if k == "file":
                            val = os.path.splitext(
                                row["info"].get("file", ""))[0]
                        else:
                            val = row["orig"].get(k, "")
                        nv = self._normalize_case(val, mode,
                                                  chk_small.isChecked())
                        if nv != val:
                            ex = (val, nv)
                            break
                if ex:
                    break
            prev.setText(
                "Voorbeeld: " + (f"“{ex[0]}” → “{ex[1]}”" if ex
                                 else "(geen wijziging)"))
        for rb in radios.values():
            rb.toggled.connect(upd_preview)
        chk_small.toggled.connect(upd_preview)
        for cb in field_chks.values():
            cb.toggled.connect(upd_preview)
        upd_preview()

        brow = QHBoxLayout()
        btn_ok = QPushButton(tr("Toepassen"))
        btn_cancel = QPushButton(tr("Annuleren"))
        btn_cancel.clicked.connect(dlg.reject)
        btn_ok.clicked.connect(dlg.accept)
        btn_ok.setDefault(True)
        brow.addStretch(1)
        brow.addWidget(btn_ok)
        brow.addWidget(btn_cancel)
        lay.addLayout(brow)
        if not dlg.exec():
            return
        mode = next(k for k, rb in radios.items() if rb.isChecked())
        keys = [k for k, cb in field_chks.items() if cb.isChecked()]
        if not keys:
            self.status_lbl.setText(tr("Geen velden gekozen."))
            return
        small = chk_small.isChecked()
        self._rebuild_row_map()
        n = 0
        self._loading = True
        try:
            for row in rows:
                for k in keys:
                    # Bestandsnaam: alleen de naam zonder extensie
                    # normaliseren, de extensie blijft zoals hij is.
                    if k == "file":
                        naam, ext = os.path.splitext(
                            row["info"].get("file", ""))
                        nv = self._normalize_case(naam, mode, small)
                        if nv and nv + ext != row["info"].get("file", ""):
                            row["rename"] = nv + ext
                            i = next((j for j, r in enumerate(self.rows)
                                      if r is row), None)
                            if i is not None:
                                fi_col = next(
                                    c for c, (kk, *_x) in enumerate(COLUMNS)
                                    if kk == "file")
                                trow = self._table_row_of.get(i)
                                if trow is not None:
                                    fi = self.table.item(trow, fi_col)
                                    if fi is not None:
                                        fi.setText(nv + ext)
                                n += 1
                        continue
                    val = row["orig"].get(k, "")
                    if not val:
                        continue
                    nv = self._normalize_case(val, mode, small)
                    if nv != val:
                        row.setdefault("changed", {})[k] = nv
                        i = next((j for j, r in enumerate(self.rows)
                                  if r is row), None)
                        if i is not None:
                            self._refresh_row_cells(i)
                        n += 1
        finally:
            self._loading = False
        self._update_changed_count()
        self.status_lbl.setText(f"🔤 {n} veld(en) genormaliseerd.")
        self._auto_save()

    # ---------- cover ----------
    def _cover_load(self):
        dlg = LopusFilePicker(
            self, mode="open", title="Cover-afbeelding kiezen",
            exts=[".jpg", ".jpeg", ".png", ".webp"])
        if not dlg.exec() or not dlg.selected:
            return
        with open(dlg.selected, "rb") as f:
            data = f.read()
        self._apply_cover(data)

    def _cover_remove(self):
        self._apply_cover(None)

    def _apply_cover(self, data):
        idxs = self._selected_rowidxs()
        if not idxs:
            self.status_lbl.setText("Eerst rijen selecteren.")
            return
        self._rebuild_row_map()
        errors = []
        for i in idxs:
            err = set_cover(self.rows[i]["path"], data)
            if err:
                errors.append(
                    f"{os.path.basename(self.rows[i]['path'])}: {err}")
            else:
                fresh = _read_info(self.rows[i]["path"])
                self.rows[i] = fresh
                r = self._table_row_of.get(i)
                if r is not None:
                    self._fill_row(r, i, fresh)
        if errors:
            self.status_lbl.setText("⚠ " + "; ".join(errors[:3]))
        else:
            n = len(idxs)
            self.status_lbl.setText(
                f"🖼 Cover {'verwijderd uit' if data is None else 'ingesteld voor'} "
                f"{n} bestand(en).")
        self._on_selection_changed()

# === DEEL 2D: bewerken / opslaan / terugdraaien ===

    # ---------- celbewerking ----------
    def _on_editor_closed(self, editor, hint):
        """Editor dicht (Enter/Esc/focusverlies): rij verversen zodat de
        kleuren kloppen met de buffer (voorkomt blijvend vals-rood)."""
        r = self.table.currentRow()
        QTimer.singleShot(0, lambda: self._refresh_after_edit(r))

    def _refresh_after_edit(self, r):
        try:
            if r is None or r < 0:
                return
            it0 = self.table.item(r, 0)
            idx = it0.data(Qt.ItemDataRole.UserRole) if it0 else None
            if idx is not None and idx < len(self.rows):
                self._refresh_row_cells(idx)
        except Exception:  # noqa: BLE001
            pass

    def _on_item_changed(self, item):
        if self._loading:
            return
        key = COLUMNS[item.column()][0]
        if key not in EDITABLE_KEYS:
            return
        idx = item.data(Qt.ItemDataRole.UserRole)
        if idx is None or idx >= len(self.rows):
            return
        row = self.rows[idx]
        new_val = item.text().strip()

        # Bestandsnaam bewerken = hernoemen op schijf (via _write_changed)
        if key == "file":
            oud = row["info"].get("file", "")
            if new_val and new_val != oud and not os.path.splitext(new_val)[1]:
                # extensie ontbreekt: die van het origineel aanvullen
                oud_ext = os.path.splitext(oud)[1]
                if oud_ext:
                    new_val = new_val + oud_ext
                    self._loading = True
                    try:
                        item.setText(new_val)
                    finally:
                        self._loading = False
            if new_val and new_val != oud:
                row["rename"] = new_val
            else:
                row.pop("rename", None)
            # Hernoemen gaat on the fly: geen rode markering nodig.
            self._loading = True
            try:
                _clear_fg(self.table, item)
            finally:
                self._loading = False
            self._update_changed_count()
            self._auto_save()
            return

        changed = new_val != row["orig"].get(key, "")
        row.setdefault("changed", {})
        # Kleur-instelling vuurt zelf ook itemChanged: guard tegen lussen.
        # Herstellen gebeurt met de standaard venster-tekstkleur (een lege
        # QColor of null-QBrush tekent zwart in donkere thema's).
        self._loading = True
        try:
            if changed:
                row["changed"][key] = new_val
                item.setForeground(QColor("#ff7070"))
            else:
                row["changed"].pop(key, None)
                _clear_fg(self.table, item)
            # Bestandsnaam-kolom markeert alleen tag-wijzigingen,
            # niet een hernoeming (die gebeurt on the fly).
            for c, (k, _t, _e, _v) in enumerate(COLUMNS):
                if k == "file":
                    fi = self.table.item(item.row(), c)
                    if fi is not None:
                        if row["changed"]:
                            fi.setForeground(QColor("#ff7070"))
                        else:
                            _clear_fg(self.table, fi)
                    break
        finally:
            self._loading = False
        self._update_changed_count()
        self._auto_save()

    # ---------- automatisch opslaan ----------
    def _auto_save(self):
        """Sla wijzigingen automatisch weg (kort uitstel zodat een reeks
        snelle bewerkingen in één keer opgeslagen wordt)."""
        QTimer.singleShot(500, self._auto_save_now)

    def _auto_save_now(self):
        if not self._changed_rows():
            return
        from PyQt6.QtWidgets import QAbstractItemView
        editing = self.table.state() == QAbstractItemView.State.EditingState
        skip = self.table.currentRow() if editing else None
        skip_c = self.table.currentColumn() if editing else None
        saved, errors = self._write_changed()
        if saved:
            # Tijdens het bewerken de actieve cel met rust laten; de rest
            # van de rijen (en de rest van de rij) wordt netjes opgeslagen.
            self._absorb_saved(saved, skip_table_row=skip, skip_col=skip_c)
            self.status_lbl.setText(
                tr("💾 {n} bestand(en) automatisch opgeslagen.").format(n=len(saved))
                + (" ⚠ " + "; ".join(errors[:2]) if errors else ""))

    def _refresh_row_cells(self, idx, skip_cell=None, skip_col=None):
        """Cellen van een rij opnieuw vullen vanuit orig+changed (rood).
        skip_cell/skip_col: cel die midden in bewerking is — alleen díé
        cel niet aanraken (de editor laat zijn eigen tekst zien)."""
        row = self.rows[idx]
        r = self._table_row_of.get(idx)
        if r is None:
            self._rebuild_row_map()
            r = self._table_row_of.get(idx)
            if r is None:
                return
        for c, (key, _t, editable, _v) in enumerate(COLUMNS):
            if not editable or key == "file":
                continue
            if r == skip_cell and c == skip_col:
                continue
            item = self.table.item(r, c)
            if item is None:
                continue
            val = row["changed"].get(key, row["orig"].get(key, ""))
            self._loading = True
            try:
                item.setText(val)
                if key in row.get("changed", {}):
                    item.setForeground(QColor("#ff7070"))
                else:
                    _clear_fg(self.table, item)
            finally:
                self._loading = False
        file_item = self.table.item(r, 0)
        if file_item is not None:
            if row.get("changed"):
                file_item.setForeground(QColor("#ff7070"))
            else:
                _clear_fg(self.table, file_item)

    def _update_changed_count(self):
        n = len(self._changed_rows())
        self.setWindowTitle("🎵 Tag-editor"
                            + (tr(" — {n} gewijzigd").format(n=n) if n else ""))

    # ---------- opslaan / terugdraaien ----------
    def _changed_rows(self):
        return [(i, row) for i, row in enumerate(self.rows)
                if row.get("changed") or row.get("rename")]

    def _write_changed(self):
        """Schrijf alle bufferwijzigingen naar schijf. Raakt de tabel niet
        aan (veilig, ook midden in een bewerking)."""
        errors, saved = [], []
        for i, row in self._changed_rows():
            try:
                # Hernoemen (Bestandsnaam-kolom bewerkt)
                newname = row.pop("rename", None)
                if newname:
                    d = os.path.dirname(row["path"])
                    ext = os.path.splitext(row["path"])[1]
                    newname = _sanitize_filename(newname)
                    # extensie niet dubbel toevoegen
                    if ext and newname.lower().endswith(ext.lower()):
                        newpath = os.path.join(d, newname)
                    else:
                        newpath = os.path.join(d, newname + ext)
                    if newpath != row["path"]:
                        if os.path.exists(newpath):
                            raise ValueError("bestaat al: "
                                             + os.path.basename(newpath))
                        os.rename(row["path"], newpath)
                        row["path"] = newpath
                        row["info"]["file"] = os.path.basename(newpath)
                if not row.get("changed"):
                    if newname is not None:
                        saved.append((i, row))
                    continue
                mf = mutagen.File(row["path"], easy=True)
                if mf is None or mf.tags is None:
                    raise ValueError("geen ondersteunde tagsoort")
                for key, val in row["changed"].items():
                    if key == "year":
                        mf.tags["date"] = val
                    elif key in ("track", "disc"):
                        mf.tags[key + "number"] = val
                    else:
                        mf.tags[key] = val
                mf.save()
                saved.append((i, row))
            except Exception as e:  # noqa: BLE001
                errors.append(f"{os.path.basename(row['path'])}: {e}")
        return saved, errors

    def _absorb_saved(self, saved, skip_table_row=None, skip_col=None):
        """Zet opgeslagen wijzigingen over naar orig (rood wordt normaal).
        skip_table_row/skip_col: cel die midden in bewerking is — alleen
        díé cel wordt met rust gelaten; de rest van de rij wordt wel
        netjes opgeslagen-kleur gegeven (anders blijven cellen vals-rood)."""
        self._loading = True
        try:
            for i, row in saved:
                r = self.rows[i]
                for k, v in row.get("changed", {}).items():
                    r["orig"][k] = v
                r["changed"] = {}
                r.pop("rename", None)
                trow = self._table_row_of.get(i)
                if trow is not None:
                    self._refresh_row_cells(
                        i, skip_cell=skip_table_row, skip_col=skip_col)
        finally:
            self._loading = False
        self._update_changed_count()

    def save_all(self):
        if not self._changed_rows():
            self.status_lbl.setText(tr("Geen wijzigingen om op te slaan."))
            return
        saved, errors = self._write_changed()
        self._loading = True
        try:
            self._rebuild_row_map()
            for i, _row in saved:
                fresh = _read_info(self.rows[i]["path"])
                self.rows[i] = fresh
                r = self._table_row_of.get(i)
                if r is not None:
                    self._fill_row(r, i, fresh)
        finally:
            self._loading = False
        if errors:
            self.status_lbl.setText(
                f"⚠ {len(errors)} fout(en): " + "; ".join(errors[:3]))
        else:
            self.status_lbl.setText(
                tr("💾 {n} bestand(en) opgeslagen.").format(n=len(saved)))
        self._update_changed_count()

    def revert(self):
        if not self._changed_rows():
            return
        self._loading = True
        try:
            self._rebuild_row_map()
            for i, _row in self._changed_rows():
                self.rows[i].pop("rename", None)
                fresh = _read_info(self.rows[i]["path"])
                self.rows[i] = fresh
                r = self._table_row_of.get(i)
                if r is not None:
                    self._fill_row(r, i, fresh)
        finally:
            self._loading = False
        self.status_lbl.setText(tr("↩️ Wijzigingen teruggedraaid."))
        self._update_changed_count()

    # ---------- rechtermuisknop-menu in de tabel ----------
    def _table_menu(self, pos):
        menu = QMenu(self)
        item = self.table.itemAt(pos)
        act_play = None
        if item is not None and item.data(Qt.ItemDataRole.UserRole + 2) is None:
            idx = item.data(Qt.ItemDataRole.UserRole)
            if idx is not None and idx < len(self.rows):
                act_play = menu.addAction(tr("▶ Openen in muziekspeler"))
                act_play.setData(idx)
        menu.addSeparator()
        m_conv = menu.addMenu(tr("🏷 Converteren"))
        a1 = m_conv.addAction("Tag - Bestandsnaam...")
        a1.triggered.connect(self._conv_tag2fn)
        a2 = m_conv.addAction("Bestandsnaam - Tag...")
        a2.triggered.connect(self._conv_fn2tag)
        a3 = m_conv.addAction("Tag - Tag...")
        a3.triggered.connect(self._conv_tag2tag)
        m_conv.addSeparator()
        a4 = m_conv.addAction("Wizard Autonummering...")
        a4.triggered.connect(self._wizard_autonum)
        menu.addSeparator()
        menu.addAction(tr("🔄 Herladen")).triggered.connect(
            lambda: self.load_folder(self.path_edit.text()))
        menu.addAction(tr("🌐 Online opzoeken...")).triggered.connect(
            lambda _checked=False: self._lookup_online())
        menu.addAction(tr("💾 Alles opslaan")).triggered.connect(self.save_all)
        menu.addAction(tr("↩️ Terugdraaien")).triggered.connect(self.revert)
        chosen = menu.exec(self.table.viewport().mapToGlobal(pos))
        if chosen is act_play and act_play is not None:
            subprocess.Popen(
                ["xdg-open", self.rows[act_play.data()]["path"]])


# === DEEL 3: online opzoeken (Discogs / MusicBrainz) ===

import urllib.request
import urllib.parse

_UA = "LopusTagEditor/1.0 (bestandsbeheerder voor Linux)"

# Gratis token aanmaken: https://www.discogs.com/settings/developers
_DISCOGS_TOKEN_URL = "https://www.discogs.com/settings/developers"


class HttpWorker(QThread):
    """Kleine HTTP-getter op de achtergrond: de UI blijft responsief."""
    got = pyqtSignal(object, str)   # (data: bytes) of (None, fouttekst)

    def __init__(self, url, headers=None, parent=None):
        super().__init__(parent)
        self._url = url
        self._headers = headers or {}

    def run(self):
        try:
            req = urllib.request.Request(
                self._url, headers={"User-Agent": _UA, **self._headers})
            with urllib.request.urlopen(req, timeout=25) as resp:
                self.got.emit(resp.read(), "")
        except Exception as e:  # noqa: BLE001
            self.got.emit(None, str(e))


def _discogs_headers(token):
    return {"Authorization": f"Discogs token={token}"}


def _discogs_search_url(artist, title, token):
    q = {"type": "release", "per_page": "30"}
    if artist:
        q["artist"] = artist
    if title:
        q["release_title"] = title
    return ("https://api.discogs.com/database/search?"
            + urllib.parse.urlencode(q) + "&token=" + urllib.parse.quote(token))


def _mb_search_url(artist, title):
    parts = []
    if artist:
        parts.append(f'artist:"{artist}"')
    if title:
        parts.append(f'release:"{title}"')
    query = urllib.parse.quote(" AND ".join(parts) or "*")
    return (f"https://musicbrainz.org/ws/2/release/?query={query}"
            "&fmt=json&limit=30")


def _mb_release_url(mbid):
    # let op: '+' in een query wordt als spatie gelezen, dus %2B
    return (f"https://musicbrainz.org/ws/2/release/{mbid}"
            "?inc=recordings%2Bmedia&fmt=json")


class TokenDialog(QDialog):
    """Mp3tag-stijl token-instelling: open de website, plak de code."""

    def __init__(self, token="", parent=None):
        super().__init__(parent)
        self.setWindowTitle(tr("Discogs-token"))
        lay = QVBoxLayout(self)
        lay.addWidget(QLabel(
            "1. Maak gratis een account op discogs.com (als je die nog\n"
            "     niet hebt).\n"
            "2. Open deze pagina en maak een 'personal access token':\n"
            f"     {_DISCOGS_TOKEN_URL}\n"
            "3. Plak de code hieronder (net zoals in Mp3tag)."))
        self.edit = QLineEdit(token)
        self.edit.setPlaceholderText(tr("plak hier je Discogs-token..."))
        lay.addWidget(self.edit)
        row = QHBoxLayout()
        btn_site = QPushButton(tr("🌐 Website openen"))
        btn_site.clicked.connect(
            lambda: subprocess.Popen(["xdg-open", _DISCOGS_TOKEN_URL]))
        btn_ok = QPushButton("OK")
        btn_ok.clicked.connect(self.accept)
        btn_weg = QPushButton(tr("Token wissen"))
        btn_weg.clicked.connect(lambda: (self.edit.clear(), self.accept()))
        row.addWidget(btn_site)
        row.addStretch(1)
        row.addWidget(btn_ok)
        row.addWidget(btn_weg)
        lay.addLayout(row)
        self.resize(520, 200)


class LookupDialog(QDialog):
    """Zoek een release op Discogs of MusicBrainz en wijs de tracklijst
    toe aan de geselecteerde bestanden. Medleys: meerdere regels
    selecteren en 'Samenvoegen' — de titels worden 'a / b / c'."""

    def __init__(self, parent, artist, album, n_files, discogs_token):
        super().__init__(parent)
        self.setWindowTitle(tr("Online opzoeken — Discogs / MusicBrainz"))
        self.resize(760, 640)
        self._token = discogs_token
        self._release = None
        self._tracks = []
        self._orig_tracks = []
        self._cover_bytes = None
        self._cover_url = None
        self._results_data = []
        self._files = []      # [(bestandsnaam, huidige titel)] voor vergelijking

        lay = QVBoxLayout(self)
        top = QHBoxLayout()
        top.addWidget(QLabel("Bron:"))
        self.src = QComboBox()
        self.src.addItems(["MusicBrainz (geen account nodig)",
                           "Discogs (token nodig)"])
        top.addWidget(self.src, 1)
        if not discogs_token:
            self.src.model().item(1).setEnabled(False)
            self.src.setToolTip(tr("Discogs is grijs: zet eerst een token (🔑)."))
        btn_tok = QPushButton(tr("🔑 Token..."))
        btn_tok.clicked.connect(self._set_token)
        top.addWidget(btn_tok)
        lay.addLayout(top)

        h = QHBoxLayout()
        h.addWidget(QLabel("Artiest:"))
        self.ed_artist = QLineEdit(artist)
        h.addWidget(self.ed_artist, 1)
        h.addWidget(QLabel("Album:"))
        self.ed_album = QLineEdit(album)
        h.addWidget(self.ed_album, 1)
        btn_zoek = QPushButton(tr("🔍 Zoeken"))
        btn_zoek.clicked.connect(self._search)
        h.addWidget(btn_zoek)
        lay.addLayout(h)

        self.status = QLabel(
            f"Zoek artiest + album en kies de release. De tracklijst wordt "
            f"vergeleken met {n_files} bestand(en) en toegepast in "
            "lijstvolgorde.")
        self.status.setWordWrap(True)
        lay.addWidget(self.status)

        self.results = QListWidget()
        self.results.setIconSize(QSize(48, 48))
        self.results.itemActivated.connect(lambda _i: self._pick_release())
        lay.addWidget(self.results, 3)

        # Tracklijst links (gescheiden kolommen) + cover-preview rechts
        tsplit = QHBoxLayout()
        self.tracks = QTableWidget(0, 4)
        self.tracks.setHorizontalHeaderLabels(
            ["Pos", "Online titel", "Pos", "Jouw titel"])
        self.tracks.verticalHeader().setVisible(False)
        self.tracks.setSelectionBehavior(
            QAbstractItemView.SelectionBehavior.SelectRows)
        self.tracks.setSelectionMode(
            QAbstractItemView.SelectionMode.ExtendedSelection)
        self.tracks.setEditTriggers(
            QAbstractItemView.EditTrigger.NoEditTriggers)
        hdr = self.tracks.horizontalHeader()
        # Interactive: breedte met de muis aan te passen
        hdr.setSectionResizeMode(QHeaderView.ResizeMode.Interactive)
        hdr.setStretchLastSection(True)
        for c, w in enumerate((46, 320, 46, 340)):
            self.tracks.setColumnWidth(c, w)
        tsplit.addWidget(self.tracks, 3)
        covcol = QVBoxLayout()
        covcol.addWidget(QLabel(tr("Cover van de gekozen release:")))
        self.cover_lbl = QLabel(tr("geen cover"))
        self.cover_lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.cover_lbl.setFixedSize(220, 220)
        self.cover_lbl.setStyleSheet(
            "border: 1px solid gray; background: #202020;")
        self.cover_lbl.setCursor(Qt.CursorShape.PointingHandCursor)
        self.cover_lbl.mousePressEvent = lambda _e: self._zoom_cover()
        covcol.addWidget(self.cover_lbl)
        covcol.addStretch(1)
        tsplit.addLayout(covcol, 1)
        lay.addLayout(tsplit, 4)

        mid = QHBoxLayout()
        btn_merge = QPushButton(tr("⏬ Samenvoegen"))
        btn_merge.setToolTip(
            "Geselecteerde trackregels samenvoegen tot één nummer:\n"
            "de titels worden 'a / b / c'. Bijv. voor een medley of\n"
            "gebabbel binnen een live-optreden dat op de schijf één\n"
            "nummer is. Met ✂ Splitsen maak je het weer ongedaan.")
        btn_merge.clicked.connect(self._merge_tracks)
        btn_split = QPushButton(tr("✂ Splitsen"))
        btn_split.setToolTip(
            "Een eerder samengevoegde regel weer uit elkaar halen naar\n"
            "de originele online tracks.")
        btn_split.clicked.connect(self._split_track)
        btn_reset = QPushButton(tr("↩️ Originele lijst"))
        btn_reset.clicked.connect(self._reset_tracks)
        mid.addWidget(btn_merge)
        mid.addWidget(btn_split)
        mid.addWidget(btn_reset)
        mid.addStretch(1)
        lay.addLayout(mid)

        self.chk_cover = QCheckBox(
            tr("Cover ook ophalen en op de bestanden zetten"))
        self.chk_cover.setChecked(True)
        lay.addWidget(self.chk_cover)

        frow = QHBoxLayout()
        frow.addWidget(QLabel("Tags schrijven:"))
        self.chk_fields = {}
        for key, lbl in (("title", "Titel"), ("artist", "Artiest"),
                         ("albumartist", "Albumartiest"),
                         ("album", "Album"), ("year", "Jaar"),
                         ("track", "Nr (1..N)")):
            cb = QCheckBox(lbl)
            cb.setChecked(key != "track")
            frow.addWidget(cb)
            self.chk_fields[key] = cb
        frow.addStretch(1)
        lay.addLayout(frow)
        self.warn_lbl = QLabel("")
        self.warn_lbl.setWordWrap(True)
        self.warn_lbl.setStyleSheet("color: orange;")
        lay.addWidget(self.warn_lbl)

        brow = QHBoxLayout()
        btn_ok = QPushButton(tr("Toepassen op bestanden"))
        btn_ok.clicked.connect(self.accept)
        btn_cancel = QPushButton(tr("Annuleren"))
        btn_cancel.clicked.connect(self.reject)
        brow.addStretch(1)
        brow.addWidget(btn_ok)
        brow.addWidget(btn_cancel)
        lay.addLayout(brow)

    # ---- hulp ----
    def _setEnabled(self, on):
        for wdg in (self.results, self.tracks):
            wdg.setEnabled(on)

    def _set_status(self, txt):
        self.status.setText(txt)

    def _busy(self, txt):
        self._setEnabled(False)
        self._set_status(txt)

    def _set_token(self):
        dlg = TokenDialog(self._token, self)
        if dlg.exec():
            token = dlg.edit.text().strip()
            self._token = token
            cfg = _load_cfg()
            cfg["discogs_token"] = token
            _save_cfg(cfg)
            self.src.model().item(1).setEnabled(bool(token))
            if token:
                self.src.setCurrentIndex(1)

    # ---- zoeken ----
    def _search(self):
        artist = self.ed_artist.text().strip()
        album = self.ed_album.text().strip()
        if not artist and not album:
            self._set_status("⚠ Vul minstens artiest of album in.")
            return
        if self.src.currentIndex() == 1:
            if not self._token:
                self._set_status("⚠ Eerst een Discogs-token instellen (🔑).")
                return
            url = _discogs_search_url(artist, album, self._token)
            hdr = _discogs_headers(self._token)
        else:
            url = _mb_search_url(artist, album)
            hdr = {}
        self._busy("🔍 Zoeken...")
        w = HttpWorker(url, hdr, self)
        w.got.connect(lambda data, err: self._search_done(data, err))
        w.start()
        self._worker = w

    def _search_done(self, data, err):
        self._setEnabled(True)
        self.results.clear()
        self._thumb_workers = []
        if err:
            self._set_status(f"⚠ Zoeken mislukt: {err}")
            return
        try:
            js = json.loads(data)
        except ValueError:
            self._set_status("⚠ Ongeldig antwoord van de server.")
            return
        items = []
        if self.src.currentIndex() == 1:
            for r in js.get("results", []):
                items.append({
                    "id": r["id"], "source": "discogs",
                    "title": r.get("title", ""), "year": r.get("year") or "",
                    "fmt": ", ".join(r.get("format") or []),
                    "label": ", ".join(r.get("label") or []),
                    "thumb": r.get("thumb") or "", "artist": "",
                    "catno": r.get("catno") or "",
                })
        else:
            for r in js.get("releases", []):
                artists = ", ".join(
                    a.get("name", "") for a in r.get("artist-credit", [])
                    if isinstance(a, dict))
                items.append({
                    "id": r["id"], "source": "mb",
                    "title": r.get("title", ""), "year": r.get("date") or "",
                    "fmt": ((r.get("release-group") or {})
                            .get("primary-type") or ""),
                    "label": "", "thumb": "", "artist": artists,
                    "catno": "",
                })
        self._results_data = items
        for it in items:
            label = f"{it['artist'] + ' — ' if it['artist'] else ''}" \
                    f"{it['title']}   [{it['year']}]  {it['fmt']}"
            if it["label"]:
                label += f"  · {it['label']}"
            QListWidgetItem(label, self.results)
        if not items:
            self._set_status("Geen resultaten gevonden.")
        else:
            self._set_status(f"{len(items)} release(s) gevonden — "
                             "dubbelklik of kies er één.")
        self._load_thumbs()

    def _load_thumbs(self):
        if self.src.currentIndex() != 1:
            return
        self._thumb_workers = []
        for n, it in enumerate(self._results_data):
            url = it.get("thumb")
            if not url:
                continue
            w = HttpWorker(url, {}, self)
            w.got.connect(
                lambda data, err, n=n: self._thumb_done(data, err, n))
            w.start()
            self._thumb_workers.append(w)

    def _thumb_done(self, data, err, n):
        if err or not data or n >= self.results.count():
            return
        pm = QPixmap()
        if pm.loadFromData(data):
            self.results.item(n).setIcon(QIcon(pm.scaled(
                48, 48, Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation)))

    # ---- release kiezen ----
    def _pick_release(self):
        n = self.results.currentRow()
        if n < 0 or n >= len(self._results_data):
            self._set_status("Eerst een release in de lijst kiezen.")
            return
        it = self._results_data[n]
        self._busy("⏳ Tracklijst ophalen...")
        if it["source"] == "discogs":
            url = f"https://api.discogs.com/releases/{it['id']}"
            hdr = _discogs_headers(self._token)
        else:
            url = _mb_release_url(it["id"])
            hdr = {}
        w = HttpWorker(url, hdr, self)
        w.got.connect(lambda data, err, it=it:
                      self._release_done(data, err, it))
        w.start()
        self._worker = w

    def _release_done(self, data, err, it):
        self._setEnabled(True)
        if err:
            self._set_status(f"⚠ Ophalen mislukt: {err}")
            from PyQt6.QtWidgets import QMessageBox
            QMessageBox.warning(
                self, "Online opzoeken",
                f"Ophalen van de release mislukt:\n{err}\n\n"
                "Probeer het nog een keer, of kies de andere bron "
                "(MusicBrainz werkt zonder token).")
            return
        try:
            js = json.loads(data)
        except ValueError:
            self._set_status("⚠ Ongeldig antwoord van de server.")
            return
        self._release = js
        self._tracks = []
        if it["source"] == "discogs":
            # soms geeft de zoekactie een 'master': die verwijst via
            # main_release naar de echte release met de tracklijst
            if not js.get("tracklist") and js.get("main_release"):
                self._busy("⏳ Echte release ophalen (master-verwijzing)...")
                w = HttpWorker(
                    f"https://api.discogs.com/releases/{js['main_release']}",
                    _discogs_headers(self._token), self)
                w.got.connect(lambda d2, e2, it=it:
                              self._release_done(d2, e2, it))
                w.start()
                self._worker = w
                return
            artist = (it.get("artist")
                      or ", ".join(a.get("name", "")
                                   for a in js.get("artists", [])))
            fmt = ", ".join(f.get("name", "")
                            for f in js.get("formats", []))
            year = str(js.get("year") or it.get("year") or "")
            # Robuust: elk item mét titel is een track; items met alleen
            # sub_tracks (kopjes/medleys) leveren hun subtracks aan.
            def _add_track(tr):
                self._tracks.append({
                    "pos": str(tr.get("position", "")),
                    "title": str(tr.get("title", "")),
                    "dur": str(tr.get("duration", "")),
                })
            for tr in js.get("tracklist", []):
                sub = tr.get("sub_tracks") or []
                if tr.get("title"):
                    _add_track(tr)
                for s in sub:
                    if s.get("title"):
                        _add_track(s)
            if not self._tracks and js.get("tracklist"):
                # onbekende structuur: laat het eerste item zien zodat
                # we het kunnen verwerken (diagnose in de statusregel)
                first = js["tracklist"][0]
                self._set_status("⚠ Tracklijst-formaat onbekend: "
                                 + ", ".join(list(first.keys())[:6]))
            imgs = js.get("images", [])
            self._cover_url = next(
                (im.get("uri") for im in imgs
                 if im.get("type") == "primary" and im.get("uri")),
                next((im.get("uri") for im in imgs if im.get("uri")),
                     None))
        else:
            artist = ", ".join(
                a.get("name", "") for a in js.get("artist-credit", [])
                if isinstance(a, dict)) or it.get("artist", "")
            media = js.get("media") or []
            fmt = ", ".join(sorted({m.get("format", "") for m in media
                                    if m.get("format")}))
            year = str(js.get("date") or it.get("year") or "")[:4]
            for m in media:
                for tr in m.get("tracks", []):
                    ms = tr.get("length") or 0
                    dur = (f"{ms // 60000}:{ms % 60000 // 1000:02d}"
                           if ms else "")
                    self._tracks.append({
                        "pos": str(tr.get("number", "")),
                        "title": tr.get("title", ""),
                        "dur": dur,
                    })
            self._cover_url = (f"https://coverartarchive.org/release/"
                               f"{it['id']}/front-500")
        self._release_meta = {"artist": artist, "album": it["title"],
                              "year": year, "fmt": fmt}
        self._orig_tracks = [dict(t) for t in self._tracks]
        self._fill_tracks()
        self._set_status(
            f"Release: {artist} — {it['title']} ({year}, {fmt or '?'}). "
            f"{len(self._tracks)} tracks. Vergelijk met je bestanden; "
            "selecteer meerdere regels + Samenvoegen voor medleys.")
        if self.chk_cover.isChecked() and self._cover_url:
            self._busy("🖼 Cover ophalen...")
            w = HttpWorker(self._cover_url, {}, self)
            w.got.connect(self._cover_done)
            w.start()
            self._worker = w

    def _cover_done(self, data, err):
        self._setEnabled(True)
        if err or not data:
            self._set_status(self.status.text() + "  (cover ophalen mislukt)")
            return
        self._cover_bytes = data
        pm = QPixmap()
        pm.loadFromData(data)
        self._cover_pm = pm
        self.cover_lbl.setPixmap(pm.scaled(
            self.cover_lbl.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation))
        self._set_status(self.status.text()
                         + f"  🖼 Cover geladen ({pm.width()}×{pm.height()}, "
                           "klik op de cover om uit te vergroten).")

    # ---- tracklijst ----
    def set_files(self, files):
        """Vergelijkingsinfo: lijst van (eigen tracknr, eigen titel)."""
        self._files = list(files)
        self._fill_tracks()

    def _fill_tracks(self):
        files = getattr(self, "_files", [])
        self.tracks.setRowCount(0)
        for n, t in enumerate(self._tracks):
            r = self.tracks.rowCount()
            self.tracks.insertRow(r)
            titel = ("⏬ " + t["title"]) if t.get("merged_from") \
                else t["title"]
            if n < len(files):
                fpos, ftitel = files[n]
                ftitel = f"“{ftitel}”" if ftitel else "(titel leeg)"
                fpos = fpos or "—"
            else:
                fpos, ftitel = "—", "— geen bestand —"
            for c, val in enumerate((t.get("pos", ""), titel,
                                     fpos, ftitel)):
                self.tracks.setItem(
                    r, c, QTableWidgetItem(str(val)))
        # waarschuwingen onderin als apart label
        warns = []
        if files:
            extra = len(files) - len(self._tracks)
            if extra > 0:
                warns.append(f"⚠ {extra} bestand(en) krijgen géén track — "
                             "selecteer regels en gebruik Samenvoegen.")
            elif extra < 0:
                warns.append(f"⚠ {abs(extra)} track(s) te veel online — "
                             "Samenvoegen tot de aantallen gelijk zijn.")
        self.warn_lbl.setText("   ".join(warns))

    def _zoom_cover(self):
        pm = getattr(self, "_cover_pm", None)
        if pm is None or pm.isNull():
            return
        dlg = QDialog(self)
        dlg.setWindowTitle(tr("Cover — klik om te sluiten"))
        lay = QVBoxLayout(dlg)
        lbl = QLabel()
        scherm = dlg.screen().availableGeometry() \
            if dlg.screen() else None
        maxw, maxh = ((scherm.width() * 7 // 10,
                       scherm.height() * 7 // 10) if scherm
                      else (900, 700))
        lbl.setPixmap(pm.scaled(
            maxw, maxh, Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation))
        lay.addWidget(lbl)
        lbl.mousePressEvent = lambda _e: dlg.accept()
        dlg.exec()

    def _zoom_cover(self):
        pm = getattr(self, "_cover_pm", None)
        if pm is None or pm.isNull():
            return
        dlg = QDialog(self)
        dlg.setWindowTitle(tr("Cover — klik om te sluiten"))
        lay = QVBoxLayout(dlg)
        lbl = QLabel()
        scherm = dlg.screen().availableGeometry() \
            if dlg.screen() else None
        maxw, maxh = ((scherm.width() * 7 // 10,
                       scherm.height() * 7 // 10) if scherm
                      else (900, 700))
        lbl.setPixmap(pm.scaled(
            maxw, maxh, Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation))
        lay.addWidget(lbl)
        lbl.mousePressEvent = lambda _e: dlg.accept()
        dlg.exec()

    def _merge_tracks(self):
        rows = sorted({i.row() for i in self.tracks.selectedIndexes()})
        if len(rows) < 2:
            self._set_status("Selecteer eerst 2 of meer trackregels om "
                             "samen te voegen.")
            return
        if any(b != a + 1 for a, b in zip(rows, rows[1:])):
            self._set_status("⚠ Alleen aaneengesloten regels samenvoegen "
                             "(zo klopt de volgorde met de schijf).")
            return
        titels = [self._tracks[r]["title"] for r in rows]
        merged = {
            "pos": self._tracks[rows[0]]["pos"],
            "title": " / ".join(titels),
            "dur": self._tracks[rows[-1]].get("dur", ""),
            "merged_from": [dict(self._tracks[r]) for r in rows],
        }
        self._tracks = (self._tracks[:rows[0]] + [merged]
                        + self._tracks[rows[-1] + 1:])
        self._fill_tracks()
        self._set_status(f"Samengevoegd tot één track: "
                         f"“{merged['title']}”.  Nog {len(self._tracks)} tracks.")

    def _split_track(self):
        """Regel splitsen: samengevoegde regel → terug naar de delen;
        gewone regel → handmatig knippen in meerdere tracks (voor bijv.
        'nummer1-nummer2-nummer3' dat bij jou 3 losse nummers zijn)."""
        rows = sorted({i.row() for i in self.tracks.selectedIndexes()})
        if len(rows) != 1:
            self._set_status("Selecteer precies één regel om te splitsen.")
            return
        r = rows[0]
        if r >= len(self._tracks):
            return
        if self._tracks[r].get("merged_from"):
            delen = [dict(t) for t in self._tracks[r]["merged_from"]]
            self._tracks = self._tracks[:r] + delen + self._tracks[r + 1:]
            self._fill_tracks()
            self._set_status(f"Regel gesplitst in {len(delen)} tracks.  "
                             f"Nog {len(self._tracks)} tracks.")
            return
        # handmatig knippen: voorstel = titel alvast gesplitst op ' / ' of ' - '
        bron = self._tracks[r]
        titel = bron["title"]
        if " / " in titel:
            voorstel = [p.strip() for p in titel.split(" / ") if p.strip()]
        else:
            voorstel = [p.strip() for p in titel.split(" - ") if p.strip()]
        dlg = QDialog(self)
        dlg.setWindowTitle(tr("Track knippen"))
        lay = QVBoxLayout(dlg)
        lay.addWidget(QLabel(
            f"Knip “{titel}” in meerdere tracks. Vul per deel de titel in\n"
            "(alvast gesplitst op ' / ' of ' -' waar dat kon):"))
        h = QHBoxLayout()
        h.addWidget(QLabel(tr("Aantal delen:")))
        n_edit = QLineEdit(str(max(1, len(voorstel))))
        n_edit.setFixedWidth(50)
        h.addWidget(n_edit)
        h.addStretch(1)
        lay.addLayout(h)
        grid = QVBoxLayout()
        edits = []

        def maak_velden(n):
            while len(edits) < n:
                e = QLineEdit(voorstel[len(edits)]
                              if len(edits) < len(voorstel) else "")
                grid.addWidget(e)
                edits.append(e)
            while len(edits) > n:
                e = edits.pop()
                grid.removeWidget(e)
                e.deleteLater()
        maak_velden(len(voorstel) or 1)
        n_edit.textChanged.connect(
            lambda t: maak_velden(max(1, min(20, int(t) if t.isdigit()
                                             else 1))))
        lay.addLayout(grid)
        brow = QHBoxLayout()
        btn_ok = QPushButton(tr("Knippen"))
        btn_cancel = QPushButton(tr("Annuleren"))
        btn_cancel.clicked.connect(dlg.reject)
        btn_ok.clicked.connect(dlg.accept)
        btn_ok.setDefault(True)
        brow.addStretch(1)
        brow.addWidget(btn_ok)
        brow.addWidget(btn_cancel)
        lay.addLayout(brow)
        if not dlg.exec():
            return
        nieuwe = []
        for e in edits:
            t = e.text().strip()
            if t:
                nieuwe.append({"pos": bron.get("pos", ""),
                               "title": t, "dur": ""})
        if len(nieuwe) < 2:
            self._set_status("Splitsen geannuleerd (minder dan 2 delen).")
            return
        self._tracks = self._tracks[:r] + nieuwe + self._tracks[r + 1:]
        self._fill_tracks()
        self._set_status(f"Regel geknipt in {len(nieuwe)} tracks.  "
                         f"Nog {len(self._tracks)} tracks.")

    def _reset_tracks(self):
        self._tracks = [dict(t) for t in self._orig_tracks]
        self._fill_tracks()
        self._set_status("Originele tracklijst hersteld.")

    # ---- resultaat ----
    def result_data(self):
        """(tracks, meta, cover_bytes|None, gekozen_velden|set())"""
        velden = {k for k, cb in self.chk_fields.items() if cb.isChecked()}
        return (self._tracks, self._release_meta,
                self._cover_bytes if self.chk_cover.isChecked() else None,
                velden)


def _tageditor_lookup(self):
    """Menu-aanroep: online opzoeken en toepassen op de selectie."""
    rows = self._conv_rows()
    if not rows:
        self.status_lbl.setText(tr("Geen bestanden om bij te werken."))
        return
    cfg = _load_cfg()
    artist = rows[0]["orig"].get("artist", "")
    album = rows[0]["orig"].get("album", "")
    dlg = LookupDialog(self, artist, album, len(rows),
                       cfg.get("discogs_token", ""))
    dlg.set_files([(r["orig"].get("track", ""),
                    r["orig"].get("title", "")) for r in rows])
    if not dlg.exec():
        return
    tracks, meta, cover, velden = dlg.result_data()
    if not tracks and not cover:
        self.status_lbl.setText(tr("Niets gekozen om toe te passen."))
        return
    alleen_cover = not velden or not tracks
    if not alleen_cover and len(tracks) != len(rows):
        self.status_lbl.setText(
            f"⚠ {len(tracks)} tracks vs. {len(rows)} bestanden — maak de "
            "lijsten eerst even lang (Samenvoegen voor medleys), of vink "
            "alle 'Tags schrijven'-vakjes uit voor alleen de cover.")
        return
    if not alleen_cover:
        self._rebuild_row_map()
        n = 0
        self._loading = True
        try:
            for volgnr, (row, tr) in enumerate(zip(rows, tracks), start=1):
                i = next((j for j, r in enumerate(self.rows)
                          if r is row), None)
                if i is None:
                    continue
                wijz = row.setdefault("changed", {})
                if "title" in velden and tr["title"] and \
                        tr["title"] != row["orig"].get("title", ""):
                    wijz["title"] = tr["title"]
                if "artist" in velden and meta.get("artist") and \
                        meta["artist"] != row["orig"].get("artist", ""):
                    wijz["artist"] = meta["artist"]
                if "albumartist" in velden and meta.get("artist") and \
                        meta["artist"] != row["orig"].get("albumartist", ""):
                    wijz["albumartist"] = meta["artist"]
                if "album" in velden and meta.get("album") and \
                        meta["album"] != row["orig"].get("album", ""):
                    wijz["album"] = meta["album"]
                if "year" in velden and meta.get("year") and \
                        meta["year"] != row["orig"].get("year", ""):
                    wijz["year"] = meta["year"]
                # Nr: opvolgend 1..N in lijstvolgorde (A1/B1-codes uit
                # Discogs zeggen niets over de volgorde op de schijf)
                if "track" in velden and \
                        str(volgnr) != row["orig"].get("track", ""):
                    wijz["track"] = str(volgnr)
                if wijz:
                    self._refresh_row_cells(i)
                    n += 1
        finally:
            self._loading = False
        self._update_changed_count()
        self.status_lbl.setText(
            f"🌐 {n} bestand(en) bijgewerkt uit "
            f"{meta.get('album', 'online')}.")
        self._auto_save()
    elif cover:
        self.status_lbl.setText(tr("🌐 Alleen de cover toepassen..."))
    if cover:
        QTimer.singleShot(600, lambda: self._lookup_cover_apply(rows, cover))


def _lookup_cover_apply(self, rows, cover):
    """Cover (al gedownload) op alle rijen van de lookup zetten."""
    ok, err = 0, []
    self._rebuild_row_map()
    for row in rows:
        e = set_cover(row["path"], cover)
        if e:
            err.append(f"{os.path.basename(row['path'])}: {e}")
        else:
            ok += 1
            i = next((j for j, r in enumerate(self.rows) if r is row), None)
            if i is not None:
                fresh = _read_info(row["path"])
                self.rows[i] = fresh
                trow = self._table_row_of.get(i)
                if trow is not None:
                    self._fill_row(trow, i, fresh)
    self.status_lbl.setText(
        self.status_lbl.text() + f"  🖼 Cover gezet op {ok} bestand(en)"
        + (f"  ⚠ {'; '.join(err[:2])}" if err else ""))


# methoden aan de TagEditor-klasse hangen
TagEditor._lookup_online = _tageditor_lookup
TagEditor._lookup_cover_apply = _lookup_cover_apply













