"""renamer.py — Massaal bestanden hernoemen.

Onderdeel van Lopus (wordt geladen door lopus.py). Werkt op een map en
toont live een voorbeeldtabel 'oude naam → nieuwe naam'; pas bij druk op
"Alles hernoemen" worden de bestanden daadwerkelijk aangepast.

Rekenvolgorde van de regels:
  1. Tekens verwijderen (vanaf positie, links/rechts)
  2. Zoeken & vervangen (met/ zonder hoofdlettergevoeligheid)
  3. Hoofdletter-stijl
  4. Opschonen (spaties → _, dubbele underscores weg)
  5. Nummeren (begin/stap/cijfers, voor- of achtervoegsel)
  6. Vast voor- / achtervoegsel

De extensie blijft standaard onaangetast (aparte optie om mee te doen).
"""
import os

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QColor
from PyQt6.QtWidgets import (
    QAbstractItemView,
    QCheckBox,
    QComboBox,
    QDialog,
    QGroupBox,
    QHBoxLayout,
    QHeaderView,
    QLabel,
    QLineEdit,
    QPushButton,
    QSplitter,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

try:
    from translations import tr
except ImportError:
    def tr(s): return s



def _split_ext(name):
    """(basisnaam, '.ext') — de laatste punt telt als extensie."""
    i = name.rfind(".")
    if i <= 0:
        return name, ""
    return name[:i], name[i:]


def title_case(s, small_ok=True):
    SMALL = {"de", "het", "een", "van", "der", "den", "en", "of", "in",
             "op", "voor", "met", "bij", "the", "and", "a", "an",
             "on", "at", "to", "for", "by"}
    words = s.split()
    out = []
    for n, w in enumerate(words):
        lw = w.lower()
        if small_ok and n > 0 and lw in SMALL:
            out.append(lw)
        else:
            out.append(lw[:1].upper() + lw[1:])
    return " ".join(out)


class Renamer(QDialog):
    """Massaal-hernoem-venster met live voorbeeld."""

    def __init__(self, parent=None, folder=None, files=None):
        super().__init__(parent)
        self.setWindowTitle(tr("✏ Massaal hernoemen"))
        self.resize(1000, 640)
        self.folder = folder or ""
        self.rows = []
        self._loading = False

        lay = QVBoxLayout(self)

        # Geen Herladen-knop meer nodig: alle regels werken live. Het
        # kleine 🔄 bij de voorbeeldtabel herleest enkel de map van schijf
        # (voor als bestanden buiten Lopus om zijn veranderd).

        split = QSplitter(Qt.Orientation.Horizontal)
        lay.addWidget(split, 1)

        left = QWidget()
        ll = QVBoxLayout(left)
        ll.setContentsMargins(0, 0, 0, 0)

        g1 = QGroupBox(tr("1 · Tekens verwijderen"))
        f1 = QHBoxLayout(g1)
        f1.addWidget(QLabel(tr("Aantal:")))
        self.ed_del_n = QLineEdit("0")
        self.ed_del_n.setFixedWidth(40)
        f1.addWidget(self.ed_del_n)
        f1.addWidget(QLabel(tr("vanaf pos.:")))
        self.ed_del_pos = QLineEdit("0")
        self.ed_del_pos.setFixedWidth(40)
        f1.addWidget(self.ed_del_pos)
        self.cmb_del_side = QComboBox()
        self.cmb_del_side.addItems(["links", "rechts"])
        f1.addWidget(self.cmb_del_side)
        f1.addStretch(1)
        ll.addWidget(g1)

        g2 = QGroupBox(tr("2 · Zoeken & vervangen"))
        f2 = QHBoxLayout(g2)
        f2.addWidget(QLabel(tr("Zoek:")))
        self.ed_find = QLineEdit()
        f2.addWidget(self.ed_find, 2)
        f2.addWidget(QLabel(tr("Vervang:")))
        self.ed_repl = QLineEdit()
        f2.addWidget(self.ed_repl, 2)
        ll.addWidget(g2)

        g3 = QGroupBox(tr("3 · Hoofdletter-stijl"))
        f3 = QHBoxLayout(g3)
        self.cmb_case = QComboBox()
        self.cmb_case.addItems([tr("— niet wijzigen —"),
                                tr("Titelstijl · Elk Woord Met Hoofdletter"),
                                tr("Zinstijl · Alleen de eerste letter groot"),
                                tr("ALLES HOOFDLETTERS"),
                                tr("alles kleine letters")])
        f3.addWidget(self.cmb_case, 1)
        self.chk_small = QCheckBox(tr("kleine woordjes laag"))
        self.chk_small.setChecked(True)
        f3.addWidget(self.chk_small)
        ll.addWidget(g3)
        self.lbl_case_ex = QLabel("")
        self.lbl_case_ex.setStyleSheet("color: gray; font-size: 11px;")
        ll.addWidget(self.lbl_case_ex)

        g4 = QGroupBox(tr("4 · Opschonen"))
        f4 = QHBoxLayout(g4)
        self.chk_spaces = QCheckBox(tr("Spaties → _"))
        self.chk_multi = QCheckBox(tr("Dubbele underscores weg"))
        f4.addWidget(self.chk_spaces)
        f4.addWidget(self.chk_multi)
        f4.addStretch(1)
        ll.addWidget(g4)

        g5 = QGroupBox(tr("5 · Nummeren"))
        f5 = QHBoxLayout(g5)
        self.chk_num = QCheckBox("Aan")
        f5.addWidget(self.chk_num)
        f5.addWidget(QLabel(tr("Begin:")))
        self.ed_num_start = QLineEdit("1")
        self.ed_num_start.setFixedWidth(42)
        f5.addWidget(self.ed_num_start)
        f5.addWidget(QLabel(tr("Stap:")))
        self.ed_num_step = QLineEdit("1")
        self.ed_num_step.setFixedWidth(42)
        f5.addWidget(self.ed_num_step)
        f5.addWidget(QLabel(tr("Cijfers:")))
        self.ed_num_dig = QLineEdit("2")
        self.ed_num_dig.setFixedWidth(42)
        f5.addWidget(self.ed_num_dig)
        self.cmb_num_pos = QComboBox()
        self.cmb_num_pos.addItems(["als voorvoegsel", "als achtervoegsel"])
        f5.addWidget(self.cmb_num_pos)
        self.ed_num_sep = QLineEdit("_")
        self.ed_num_sep.setFixedWidth(34)
        f5.addWidget(self.ed_num_sep)
        f5.addStretch(1)
        ll.addWidget(g5)

        g6 = QGroupBox(tr("6 · Vast voor-/achtervoegsel"))
        f6 = QHBoxLayout(g6)
        f6.addWidget(QLabel(tr("Voor:")))
        self.ed_prefix = QLineEdit()
        f6.addWidget(self.ed_prefix, 1)
        f6.addWidget(QLabel(tr("Achter:")))
        self.ed_suffix = QLineEdit()
        f6.addWidget(self.ed_suffix, 1)
        ll.addWidget(g6)

        gb_opt = QGroupBox(tr("Opties"))
        fo = QHBoxLayout(gb_opt)
        self.chk_ext = QCheckBox(tr("Extensie óók aanpassen"))
        self.chk_ext.setChecked(False)
        fo.addWidget(self.chk_ext)
        self.chk_case_sens = QCheckBox(tr("Zoeken hoofdlettergevoelig"))
        fo.addWidget(self.chk_case_sens)
        fo.addStretch(1)
        ll.addWidget(gb_opt)

        # live herberekenen bij elke wijziging — MOET na de UI-bouw:
        # nu hebben alle widgets een parent en zijn ze vindbaar
        for le in self.findChildren(QLineEdit):
            le.textChanged.connect(self.recompute)
        for cb in (self.chk_small, self.chk_spaces, self.chk_multi,
                   self.chk_num, self.chk_ext, self.chk_case_sens):
            cb.toggled.connect(self.recompute)
        for cmb in (self.cmb_case, self.cmb_del_side, self.cmb_num_pos):
            cmb.currentIndexChanged.connect(self.recompute)

        split.addWidget(left)

        # ---------- rechterkolom: voorbeeld ----------
        right = QWidget()
        rl = QVBoxLayout(right)
        rl.setContentsMargins(0, 0, 0, 0)
        top_r = QHBoxLayout()
        btn_all = QPushButton(tr("☑ Alles aan"))
        btn_all.clicked.connect(lambda: self._toggle_all(True))
        btn_none = QPushButton(tr("☐ Alles uit"))
        btn_none.clicked.connect(lambda: self._toggle_all(False))
        top_r.addWidget(btn_all)
        top_r.addWidget(btn_none)
        top_r.addStretch(1)
        self.path_lbl = QLabel(self.folder or "-")
        self.path_lbl.setStyleSheet("color: gray;")
        top_r.addWidget(self.path_lbl, 1)
        btn_preview = QPushButton("🔄")
        btn_preview.setToolTip(
            "Map opnieuw van schijf inlezen (alleen nodig als bestanden "
            "buiten Lopus om zijn veranderd)")
        btn_preview.setFixedWidth(30)
        btn_preview.clicked.connect(lambda: self.reload())
        top_r.addWidget(btn_preview)
        rl.addLayout(top_r)

        self.table = QTableWidget(0, 3)
        self.table.setHorizontalHeaderLabels(
            ["≈", "Oude naam", "Nieuwe naam"])
        self.table.verticalHeader().setVisible(False)
        self.table.setEditTriggers(
            QAbstractItemView.EditTrigger.NoEditTriggers)
        th = self.table.horizontalHeader()
        th.setSectionResizeMode(1, QHeaderView.ResizeMode.Interactive)
        th.setSectionResizeMode(2, QHeaderView.ResizeMode.Stretch)
        self.table.setColumnWidth(0, 26)
        self.table.setColumnWidth(1, 320)
        self.table.itemClicked.connect(self._on_row_click)
        rl.addWidget(self.table, 1)

        self.status = QLabel("")
        rl.addWidget(self.status)

        brow = QHBoxLayout()
        btn_apply = QPushButton(tr("✏ Alles hernoemen"))
        btn_apply.clicked.connect(self._apply)
        btn_close = QPushButton(tr("Sluiten"))
        btn_close.clicked.connect(self.close)
        brow.addStretch(1)
        brow.addWidget(btn_apply)
        brow.addWidget(btn_close)
        rl.addLayout(brow)

        split.addWidget(right)
        split.setSizes([430, 550])

        # hulpregels + tooltips
        self.ed_find.setToolTip(
            tr("De tekst die gezocht wordt in elke bestandsnaam."))
        self.chk_case_sens.setToolTip(
            tr("Aan:  'Faith' vindt alleen 'Faith' (exact dezelfde letters).\n"
               "Uit:  'faith' vindt ook 'Faith' en 'FAITH' (hoofdletters "
               "maken niet uit)."))
        self.chk_ext.setToolTip(
            tr("Uit (standaard): '.mp3' enz. blijven altijd zoals ze zijn.\n"
               "Aan: de zoek/vervang- en stijlregels gelden óók voor de "
               "extensie."))

        def update_case_example(*_a):
            mode = self.cmb_case.currentIndex()
            bron = "de dag van mijn vakantie"
            if mode == 0:
                self.lbl_case_ex.setText("")
            elif mode == 1:
                uit = title_case(bron, self.chk_small.isChecked())
                self.lbl_case_ex.setText(
                    f"Titelstijl: elk woord met hoofdletter   "
                    f"bijv. “{bron}” → “{uit}”")
            elif mode == 2:
                uit = bron[:1].upper() + bron[1:].lower()
                self.lbl_case_ex.setText(
                    f"Zinstijl: alléén de eerste letter groot   "
                    f"bijv. “{bron}” → “{uit}”")
            elif mode == 3:
                self.lbl_case_ex.setText(
                    f"ALLES HOOFDLETTERS   "
                    f"bijv. “{bron}” → “{bron.upper()}”")
            else:
                self.lbl_case_ex.setText(
                    f"alles kleine letters   "
                    f"bijv. “{bron}” → “{bron.lower()}”")

        for cmb in (self.cmb_case,):
            cmb.currentIndexChanged.connect(update_case_example)
        self.chk_small.toggled.connect(update_case_example)
        update_case_example()

        # live herberekenen bij elke wijziging — ná de UI-bouw, zodat
        # findChildren alle velden vindt (de widgets hebben pas nu een parent)
        for le in self.findChildren(QLineEdit):
            le.textChanged.connect(self.recompute)
        for cb in (self.chk_small, self.chk_spaces, self.chk_multi,
                   self.chk_num, self.chk_ext, self.chk_case_sens):
            cb.toggled.connect(self.recompute)
        for cmb in (self.cmb_case, self.cmb_del_side, self.cmb_num_pos):
            cmb.currentIndexChanged.connect(self.recompute)

        if files:
            self.set_files(files)
        elif folder:
            self.reload()

    # ---------- data ----------
    def set_files(self, paths):
        """Bestandslijst rechtstreeks zetten (bijv. uit een selectie)."""
        self.rows = [{"path": p,
                      "old_name": os.path.basename(p),
                      "checked": True}
                     for p in paths]
        self.folder = os.path.dirname(paths[0]) if paths else ""
        self.path_lbl.setText(self.folder)
        self.recompute()

    def reload(self):
        if not self.folder or not os.path.isdir(self.folder):
            self.status.setText(tr("⚠ Geen geldige map."))
            return
        try:
            entries = sorted(os.scandir(self.folder),
                             key=lambda e: e.name.lower())
        except OSError as e:
            self.status.setText(
                tr("⚠ Map lezen mislukt: {err}").format(err=e))
            return
        self.rows = []
        for e in entries:
            if e.is_file(follow_symlinks=False):
                self.rows.append({"path": e.path,
                                  "old_name": e.name,
                                  "checked": True})
        self.recompute()

    def _toggle_all(self, on):
        self._loading = True
        for r in range(self.table.rowCount()):
            it = self.table.item(r, 0)
            if it is not None:
                it.setCheckState(
                    Qt.CheckState.Checked if on
                    else Qt.CheckState.Unchecked)
        self._loading = False
        self._sync_checks()

    def _on_row_click(self, item):
        """Klik op het ≈-vakje wisselt de rij aan/uit."""
        if item.column() == 0 and not self._loading:
            self._sync_checks()

    def _sync_checks(self):
        for r in range(self.table.rowCount()):
            it = self.table.item(r, 0)
            if it is not None and r < len(self.rows):
                self.rows[r]["checked"] = \
                    it.checkState() == Qt.CheckState.Checked

    # ---------- regels toepassen ----------
    def _new_name(self, old, volgnr):
        """Pas alle regels toe op één naam (in vaste volgorde)."""
        if self.chk_ext.isChecked():
            naam, ext = old, ""
        else:
            naam, ext = _split_ext(old)

        # 1 · tekens verwijderen
        try:
            n = int(self.ed_del_n.text().strip() or "0")
            pos = int(self.ed_del_pos.text().strip() or "0")
        except ValueError:
            n = pos = 0
        if n > 0 and pos < len(naam):
            if self.cmb_del_side.currentIndex() == 0:
                naam = naam[:pos] + naam[pos + n:]
            else:
                einde = max(0, len(naam) - pos)
                naam = naam[:max(0, einde - n)] + naam[einde:]

        # 2 · zoeken & vervangen
        vind = self.ed_find.text()
        if vind:
            repl = self.ed_repl.text()
            if self.chk_case_sens.isChecked():
                naam = naam.replace(vind, repl)
            else:
                low, up = naam.lower(), vind.lower()
                uit, i = "", 0
                while True:
                    j = low.find(up, i)
                    if j < 0:
                        uit += naam[i:]
                        break
                    uit += naam[i:j] + repl
                    i = j + len(vind)
                naam = uit

        # 3 · hoofdletters
        mode = self.cmb_case.currentIndex()
        if mode == 1:
            naam = title_case(naam, self.chk_small.isChecked())
        elif mode == 2:
            naam = naam[:1].upper() + naam[1:].lower()
        elif mode == 3:
            naam = naam.upper()
        elif mode == 4:
            naam = naam.lower()

        # 4 · opschonen
        if self.chk_spaces.isChecked():
            naam = naam.replace(" ", "_")
        if self.chk_multi.isChecked():
            while "__" in naam:
                naam = naam.replace("__", "_")

        # 5 · nummeren
        if self.chk_num.isChecked():
            try:
                start = int(self.ed_num_start.text().strip() or "1")
                step = int(self.ed_num_step.text().strip() or "1")
                dig = max(1, min(6,
                                 int(self.ed_num_dig.text().strip()
                                     or "2")))
            except ValueError:
                start, step, dig = 1, 1, 2
            num = str(start + volgnr * step).zfill(dig)
            sep = self.ed_num_sep.text()
            if self.cmb_num_pos.currentIndex() == 0:
                naam = num + sep + naam
            else:
                naam = naam + sep + num

        # 6 · vast voor-/achtervoegsel
        pre, suf = self.ed_prefix.text(), self.ed_suffix.text()
        if pre:
            naam = pre + naam
        if suf:
            naam = naam + suf
        return naam + ext

    # ---------- live voorbeeld ----------
    def recompute(self):
        self._loading = True
        try:
            self.table.setRowCount(len(self.rows))
            n_wijz = 0
            dups = set()
            gezien = {}
            for r, row in enumerate(self.rows):
                nieuw = self._new_name(row["old_name"], r)
                row["new_name"] = nieuw
                low = nieuw.lower()
                if low in gezien and \
                        gezien[low] != row["old_name"].lower():
                    dups.add(r)
                    dups.add(gezien[low])
                elif low != row["old_name"].lower():
                    gezien[low] = row["old_name"].lower()
            for r, row in enumerate(self.rows):
                nieuw = row.get("new_name",
                                self._new_name(row["old_name"], r))
                ok = nieuw != row["old_name"]
                if ok:
                    n_wijz += 1
                chk = QTableWidgetItem()
                chk.setCheckState(
                    Qt.CheckState.Checked if row.get("checked", True)
                    else Qt.CheckState.Unchecked)
                chk.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
                self.table.setItem(r, 0, chk)
                self.table.setItem(
                    r, 1, QTableWidgetItem(row["old_name"]))
                nieuw_it = QTableWidgetItem(nieuw)
                if r in dups:
                    nieuw_it.setForeground(QColor("#ff5555"))
                elif ok:
                    nieuw_it.setForeground(QColor("#8fdc8f"))
                self.table.setItem(r, 2, nieuw_it)
        finally:
            self._loading = False
        self.status.setText(
            tr("{n} bestanden · {m} worden hernoemd").format(
                n=len(self.rows), m=n_wijz)
            + (tr("   ⚠ dubbele namen in rood!") if dups else ""))

    # ---------- uitvoeren ----------
    def _apply(self):
        self._sync_checks()
        todo = [(row["path"], row["new_name"])
                for row in self.rows
                if row.get("checked")
                and row.get("new_name") and row["new_name"] != row["old_name"]]
        nieuw_lows = [n.lower() for _p, n in todo]
        if len(set(nieuw_lows)) != len(nieuw_lows):
            self.status.setText(
                tr("⚠ Er staan dubbele nieuwe namen tussen — los die eerst op "
                "(staan in rood)."))
            return
        if not todo:
            self.status.setText(tr("Niets te hernoemen."))
            return
        errors, ok = [], 0
        for pad, nieuw in todo:
            doel = os.path.join(os.path.dirname(pad), nieuw)
            try:
                os.rename(pad, doel)
                ok += 1
            except OSError as e:
                errors.append(f"{os.path.basename(pad)}: {e}")
        self.reload()
        self.status.setText(
            tr("✏ {n} bestand(en) hernoemd.").format(n=ok)
            + (f"  ⚠ {'; '.join(errors[:3])}" if errors else ""))
