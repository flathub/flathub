"""Lopus FTP/SFTP-client.

Verbinding via FTP, FTPS (explicit TLS) of SFTP. Links de lokale
computer, rechts de server. Dubbelklik = map in. Knoppen voor uploaden,
downloaden, verwijderen en hernoemen. Onthoudt host/gebruiker in ~/.config/lopus/ftp.json.
"""
import os
import socket
import ssl
import posixpath
import stat as statmod
import json
from ftplib import error_perm, error_temp

from PyQt6.QtCore import Qt, QThread, pyqtSignal, QTimer
from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit,
    QPushButton, QComboBox, QListWidget, QListWidgetItem, QMessageBox,
    QInputDialog, QProgressBar, QDialogButtonBox, QApplication,
    QCheckBox, QAbstractItemView,
)

try:
    from translations import tr
except ImportError:
    def tr(s): return s


try:
    import paramiko  # type: ignore
except ImportError:
    paramiko = None

_LOCAL_CFG = os.path.expanduser("~/.config/lopus/ftp.json")
APP_TITLE = "Lopus — FTP/SFTP"


def _cfg_path():
    return _LOCAL_CFG


def load_remembered():
    try:
        with open(_cfg_path()) as f:
            return json.load(f)
    except Exception:  # noqa: BLE001
        return {}


def save_remembered(data):
    try:
        os.makedirs(os.path.dirname(_cfg_path()), exist_ok=True)
        with open(_cfg_path(), "w") as f:
            json.dump(data, f, indent=1)
    except Exception:  # noqa: BLE001
        pass


def human(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n:.0f} {unit}" if unit == "B" else f"{n:.1f} {unit}"
        n /= 1024
    return f"{n:.1f} TB"


def _parse_list(lines, base):
    """'LIST'-regels naar (naam, pad, isdir, size); unix-stijl."""
    out = []
    for line in lines:
        parts = line.split()
        if len(parts) < 9 or parts[0].lower().startswith("total"):
            continue
        perms, size = parts[0], parts[4]
        name = line[line.find(parts[8]):].strip()
        if name in (".", ".."):
            continue
        isdir = perms[0] == "d" or perms.startswith("l")
        try:
            size = int(size)
        except ValueError:
            size = 0
        out.append((name, posixpath.join(base, name), isdir, size))
    return sorted(out, key=lambda o: (not o[2], o[0].lower()))


class Client:
    """Uniforme wrapper rond FTP / FTPS / SFTP."""

    @staticmethod
    def _ftps_context():
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE       # self-signed servercert
        ctx.maximum_version = ssl.TLSVersion.TLSv1_2  # sessie-hergebruik
        return ctx

    def __init__(self):
        self.kind = None
        self.ftp = None
        self.sftp = None
        self.ssh = None

    def connect(self, proto, host, port, user, password,
                no_prot_p=False):
        self._args = (proto, host, port, user, password)
        if proto == "SFTP":
            if paramiko is None:
                raise RuntimeError(
                    "paramiko is niet geïnstalleerd:\n"
                    "pip install --user --break-system-packages paramiko")
            self.kind = "sftp"
            self.ssh = paramiko.SSHClient()
            self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            kw = {"username": user or None, "timeout": 20}
            if password:
                kw["password"] = password
            self.ssh.connect(host, port or 22, **kw)
            self.sftp = self.ssh.open_sftp()
            try:
                self.sftp.settimeout(30)  # voorkom eeuwig wachten bij close
            except Exception:  # noqa: BLE001
                pass
            try:
                home = self.sftp.normalize(".")
            except Exception:  # noqa: BLE001
                home = "/"
            return home
        from ftplib import FTP_TLS, FTP
        self.kind = "ftp"
        # FileZilla Server eist TLS-sessie-hergebruik op het datakanaal.
        # Onder TLS 1.3 deelt Python's ssl-module sessies niet over sockets
        # ("TLS session of data connection not resumed") — forukt daarom
        # naar max. TLS 1.2, zoals de FileZilla-client ook doet.
        cls = FTP_TLS if proto == "FTPS" else FTP
        self.ftp = cls() if proto != "FTPS" else cls(
            context=self._ftps_context())
        self.ftp.connect(host, port or 21, timeout=25)
        self.ftp.login(user or "anonymous", password or "")
        if proto == "FTPS" and not no_prot_p:
            self.ftp.prot_p()
            self.ftp.prot_p_set = True
        pwd = self.ftp.pwd()
        try:
            self.ftp.sendcmd("TYPE I")
        except Exception:  # noqa: BLE001
            pass
        return pwd

    def close(self):
        for obj in (self.sftp, self.ssh, self.ftp):
            try:
                if obj:
                    obj.close()
            except Exception:  # noqa: BLE001
                pass
        self.sftp = self.ssh = self.ftp = None

    def listdir(self, path):
        out = []
        if self.kind == "sftp":
            for attr in self.sftp.listdir_attr(path):
                p = posixpath.join(path, attr.filename)
                out.append((attr.filename, p,
                            statmod.S_ISDIR(attr.st_mode or 0),
                            getattr(attr, "st_size", 0) or 0))
            dirs = [o for o in out if o[2]]
            files = sorted((o for o in out if not o[2]),
                           key=lambda o: o[0].lower())
            return dirs + files
        lines = []
        try:
            self.ftp.retrlines("LIST", lines.append)
        except (OSError, error_temp, error_perm) as exc:
            # FTPS-herstel: sommige servers weigeren TLS-sessie-hergebruik op
            # het datakanaal (Errno 104).  Val dan terug op onversleuteld
            # datakanaal (PROT C); het besturingskanaal blijft versleuteld.
            if getattr(self.ftp, "prot_p_set", False):
                try:
                    self.ftp.prot_c()
                    self.ftp.prot_p_set = False
                    lines = []
                    self.ftp.retrlines("LIST", lines.append)
                except Exception:  # noqa: BLE001
                    raise exc
            else:
                raise
        return _parse_list(lines, path)

    def exists(self, path):
        try:
            if self.kind == "sftp":
                self.sftp.stat(path)
            else:
                self.ftp.size(path)
            return True
        except Exception:  # noqa: BLE001
            return False

    def mkdir(self, path):
        if self.kind == "sftp":
            self.sftp.mkdir(path)
        else:
            self.ftp.mkd(path)

    def mkdir_p(self, path):
        """Maak (externe) map aan, inclusief ontbrekende ouders.
        Gooit een fout als de map daarna alsnog niet bestaat."""
        path = path.rstrip("/")
        parts = [p for p in path.split("/") if p]
        cur = "/" if path.startswith("/") else ""
        for part in parts:
            cur = cur + part if cur in ("", "/") else cur + "/" + part
            if not self.exists(cur):
                try:
                    self.mkdir(cur)
                except Exception as e:  # noqa: BLE001
                    raise RuntimeError(
                        f"Kan map '{cur}' niet aanmaken op de server: {e}")
            if not self.exists(cur):
                raise RuntimeError(
                    f"Map '{cur}' bestaat (nog) niet na aanmaakpoging — "
                    "controleer de rechten op de server.")

    def walk_remote(self, path):
        """Recursief: levert (volledig_pad, isdir, size) van alles onder path."""
        for name, full, isdir, size in self.listdir(path):
            if isdir:
                yield full, True, 0
                yield from self.walk_remote(full)
            else:
                yield full, False, size

    def walk_local(self, path):
        """Recursief: levert (volledig_pad, isdir) van alles onder path."""
        for root, _dirs, files in os.walk(path):
            rel = os.path.relpath(root, path)
            if rel != ".":
                yield root.replace(os.sep, "/"), True
            for f in files:
                yield os.path.join(root, f).replace(os.sep, "/"), False

    def delete(self, path, isdir):
        if self.kind == "sftp":
            if isdir:
                for name, full, d, _sz in self.listdir(path):
                    self.delete(full, d)
                self.sftp.rmdir(path)
            else:
                self.sftp.remove(path)
        elif isdir:
            self.ftp.rmd(path)
        else:
            self.ftp.delete(path)

    def rename(self, path, new_name):
        new = posixpath.join(posixpath.dirname(path), new_name) \
            if "/" in path.rstrip("/") else new_name
        if self.kind == "sftp":
            self.sftp.rename(path, new)
        else:
            self.ftp.rename(path, new)
        return new

    def download(self, remote, local, progress=None, chunk=1024 * 1024,
                 should_stop=None):
        done = 0
        if self.kind == "sftp":
            try:
                total = self.sftp.stat(remote).st_size or -1
            except Exception:  # noqa: BLE001
                total = -1
            with self.sftp.open(remote, "rb") as rf, open(local, "wb") as lf:
                import time as _ts
                while True:
                    if should_stop and should_stop():
                        raise InterruptedError("Afgebroken")
                    buf = rf.read(chunk)
                    if not buf:
                        break
                    lf.write(buf)
                    done += len(buf)
                    if progress and total > 0:
                        progress(min(100, int(done * 100 / total)))
                    _ts.sleep(0.003)  # GIL teruggeven: UI blijft levend
        else:
            try:
                total = self.ftp.size(remote) or -1
            except Exception:  # noqa: BLE001
                total = -1
            with open(local, "wb") as lf:

                def cb(b):
                    nonlocal done
                    if should_stop and should_stop():
                        raise InterruptedError("Afgebroken")
                    lf.write(b)
                    done += len(b)
                    if progress and total > 0:
                        progress(min(100, int(done * 100 / total)))
                self.ftp.retrbinary(f"RETR {remote}", cb, blocksize=chunk)

    def upload(self, local, remote, progress=None, chunk=1024 * 1024,
               should_stop=None):
        total = max(os.path.getsize(local), 1)
        done = 0
        if self.kind == "sftp":
            with open(local, "rb") as lf, self.sftp.open(remote, "wb") as rf:
                import time as _ts
                while True:
                    if should_stop and should_stop():
                        raise InterruptedError("Afgebroken")
                    buf = lf.read(chunk)
                    if not buf:
                        break
                    rf.write(buf)
                    done += len(buf)
                    if progress:
                        progress(min(100, int(done * 100 / total)))
                    _ts.sleep(0.003)  # GIL teruggeven: UI blijft levend
        else:
            with open(local, "rb") as lf:

                def cb(b):
                    nonlocal done
                    if should_stop and should_stop():
                        raise InterruptedError("Afgebroken")
                    done += len(b)
                    if progress:
                        progress(min(100, int(done * 100 / total)))
                self.ftp.storbinary(f"STOR {remote}", lf,
                                    blocksize=chunk, callback=cb)


class _Job(QThread):
    """Voert netwerkwerk op de achtergrond uit; UI bevriest niet."""
    done = pyqtSignal(object)
    prog = pyqtSignal(int)
    speed = pyqtSignal(str)

    def __init__(self, fn, parent=None):
        super().__init__(parent)
        self.fn = fn
        self._cancelled = False

    def cancel(self):
        self._cancelled = True

    def should_stop(self):
        return self._cancelled

    def run(self):
        try:
            socket.setdefaulttimeout(30)
        except Exception:
            pass

        try:
            # Geef eventueel should_stop mee als de functie dat accepteert
            # (of handhaaf via een lambda/closure in je UI)
            r = self.fn()
        except Exception as e:
            r = e
        self.done.emit(r)


class FtpDialog(QDialog):
    """FileZilla-achtig venster: lokale kolom links, server rechts."""

    def __init__(self, parent=None, start_local=None):
        super().__init__(parent)
        self.setWindowTitle(APP_TITLE)
        self.resize(980, 560)
        self.cli = Client()
        self.remote_path = "/"
        vlay = QVBoxLayout(self)

        conn = QHBoxLayout()
        saved = load_remembered()
        self.book = saved.get("book", []) \
            if isinstance(saved.get("book"), list) else []
        self.cmb_book = QComboBox()
        self._fill_book(saved)
        self.btn_save = QPushButton("💾")
        self.btn_save.setFixedSize(34, 28)
        self.btn_save.setToolTip(tr("Huidige verbinding opslaan in adresboek"))
        self.btn_delb = QPushButton("🗑")
        self.btn_delb.setFixedSize(34, 28)
        self.btn_delb.setToolTip(tr("Geselecteerde verbinding uit adresboek "
                                 "verwijderen"))
        self.cmb_book.currentIndexChanged.connect(self._pick_book)
        self.btn_save.clicked.connect(self._save_book_entry)
        self.btn_delb.clicked.connect(self._del_book_entry)
        self.cmb_proto = QComboBox()
        self.cmb_proto.addItems(["SFTP", "FTPS", "FTP"])
        self.txt_host = QLineEdit()
        self.txt_host.setPlaceholderText(tr("server.nl"))
        self.txt_port = QLineEdit()
        self.txt_port.setFixedWidth(56)
        self.txt_user = QLineEdit()
        self.txt_pass = QLineEdit()
        self.txt_pass.setEchoMode(QLineEdit.EchoMode.Password)
        conn.addWidget(QLabel(tr("Overdracht:")))
        self.cmb_mode = QComboBox()
        self.cmb_mode.addItems([tr("Passief"), tr("Actief")])
        self.cmb_mode.setFixedWidth(90)
        self.cmb_mode.setToolTip(
            tr("Passief = server opent de data-verbinding (standaard).\n"
               "Actief = jouw pc opent de data-verbinding (gebruik dit bij\n"
               "bijv. een VPN die passieve verbindingen blokkeert)."))
        self.btn_conn = QPushButton(tr("🔌 Verbinden"))
        self.btn_disc = QPushButton(tr("⏏ Verbreken"))
        self.btn_disc.setEnabled(False)
        for i, lbl in enumerate((tr("Type"), tr("Server"), tr("Poort"),
                                 tr("Gebruiker"), tr("Wachtwoord"))):
            conn.addWidget(QLabel(lbl))
            conn.addWidget((self.cmb_proto, self.txt_host, self.txt_port,
                            self.txt_user, self.txt_pass)[i])
        conn.addWidget(self.cmb_mode)
        conn.addWidget(self.btn_conn)
        conn.addWidget(self.btn_disc)
        brow = QHBoxLayout()
        brow.setContentsMargins(0, 0, 0, 0)
        lbl_book = QLabel(tr("Adresboek:"))
        lbl_book.setStyleSheet("font-weight: bold;")
        brow.addWidget(lbl_book)
        brow.addWidget(self.cmb_book, 1)
        brow.addWidget(self.btn_save)
        brow.addWidget(self.btn_delb)
        vlay.addLayout(brow)
        self.btn_save.setVisible(False)  # pas zichtbaar bij echte selectie
        self.btn_delb.setVisible(False)
        vlay.addLayout(conn)

        cols = QHBoxLayout()
        left_col = QVBoxLayout()
        left_col.addWidget(QLabel(tr("💻 Deze computer")))
        self.chk_hidden = QCheckBox(tr("Verborgen items tonen"))
        self.chk_hidden.setChecked(False)
        self.chk_hidden.toggled.connect(lambda _v: self.fill_local())
        left_col.addWidget(self.chk_hidden)
        self.lst_local = QListWidget()
        self.lst_local.setSelectionMode(
            QAbstractItemView.SelectionMode.ExtendedSelection)
        left_col.addWidget(self.lst_local)
        self.lbl_local = QLabel("")
        left_col.addWidget(self.lbl_local)

        mid = QVBoxLayout()
        mid.addStretch(1)
        self.btn_up = QPushButton(tr("⬆ Uploaden →"))
        self.btn_down = QPushButton(tr("← Downloaden"))
        self.btn_del_r = QPushButton(tr("🗑 Verwijderen"))
        self.btn_ren_r = QPushButton(tr("✎ Hernoemen"))
        for b in (self.btn_up, self.btn_down, self.btn_del_r, self.btn_ren_r):
            b.setEnabled(False)
            mid.addWidget(b)
        mid.addStretch(1)

        right_col = QVBoxLayout()
        self.lbl_remote = QLabel(tr("🌐 Server — niet verbonden"))
        right_col.addWidget(self.lbl_remote)
        self.lst_remote = QListWidget()
        self.lst_remote.setSelectionMode(
            QAbstractItemView.SelectionMode.ExtendedSelection)
        right_col.addWidget(self.lst_remote)
        cols.addLayout(left_col)
        cols.addLayout(mid)
        cols.addLayout(right_col)
        vlay.addLayout(cols, 1)

        self.bar = QProgressBar()
        self.bar.setFixedHeight(14)
        self.bar.hide()
        self.btn_cancel = QPushButton(tr("⛔ Stop"))
        self.btn_cancel.setFixedHeight(14)
        self.btn_cancel.hide()
        self.btn_cancel.clicked.connect(self._cancel_transfer)
        barrow = QHBoxLayout()
        barrow.addWidget(self.bar, 1)
        barrow.addWidget(self.btn_cancel)
        vlay.addLayout(barrow)
        row = QHBoxLayout()
        row.addStretch(1)
        bb = QDialogButtonBox(QDialogButtonBox.StandardButton.Close)
        bb.button(QDialogButtonBox.StandardButton.Close).setText(
            tr("Sluiten"))
        bb.rejected.connect(self.reject)
        row.addWidget(bb)
        vlay.addLayout(row)
        self.status_lbl = QLabel("")
        vlay.addWidget(self.status_lbl)

        self.btn_conn.clicked.connect(self.connect_now)
        self.btn_disc.clicked.connect(self.disconnect_now)
        self.lst_local.itemDoubleClicked.connect(self.local_navigate)
        self.lst_local.setContextMenuPolicy(
            Qt.ContextMenuPolicy.CustomContextMenu)
        self.lst_local.customContextMenuRequested.connect(self.local_menu)
        self.lst_remote.itemDoubleClicked.connect(self.remote_navigate)
        self.lst_remote.setContextMenuPolicy(
            Qt.ContextMenuPolicy.CustomContextMenu)
        self.lst_remote.customContextMenuRequested.connect(
            self.remote_menu)
        self.btn_up.clicked.connect(lambda: self.transfer(True))
        self.btn_down.clicked.connect(lambda: self.transfer(False))
        self.btn_del_r.clicked.connect(self.del_remote)
        self.btn_ren_r.clicked.connect(self.ren_remote)

        self._local_path = start_local or os.path.expanduser("~")
        self.fill_local()

    def status(self, txt):
        self.status_lbl.setText(txt)

    # ---------- rechtermuisknop-menu's ----------
    def local_menu(self, pos):
        from PyQt6.QtGui import QCursor
        it = self.lst_local.itemAt(pos)
        if not it:
            return
        path, isdir = it.data(Qt.ItemDataRole.UserRole)
        if path and os.path.basename(path) == "..":
            return
        from PyQt6.QtWidgets import QMenu
        menu = QMenu(self)
        act_del = menu.addAction(tr("🗑 Verwijderen"))
        act_ren = menu.addAction(tr("✎ Hernoemen"))
        act = menu.exec(QCursor.pos())
        if act == act_del:
            if QMessageBox.question(
                    self, APP_TITLE,
                    f"'{os.path.basename(path)}' verwijderen?"
            ) != QMessageBox.StandardButton.Yes:
                return
            try:
                if isdir:
                    import shutil
                    shutil.rmtree(path)
                else:
                    os.remove(path)
                self.fill_local()
                self.status("Verwijderd ✓")
            except OSError as e:
                QMessageBox.warning(self, APP_TITLE, str(e))
        elif act == act_ren:
            name, ok = QInputDialog.getText(self, APP_TITLE, "Nieuwe naam:",
                                            text=os.path.basename(path))
            if ok and name.strip() and name.strip() != os.path.basename(path):
                try:
                    os.rename(path, os.path.join(
                        os.path.dirname(path), name.strip()))
                    self.fill_local()
                    self.status("Hernoemd ✓")
                except OSError as e:
                    QMessageBox.warning(self, APP_TITLE, str(e))

    def remote_menu(self, pos):
        from PyQt6.QtGui import QCursor
        from PyQt6.QtWidgets import QMenu
        it = self.lst_remote.itemAt(pos)
        if not it or not self.cli.kind:
            return
        data = it.data(Qt.ItemDataRole.UserRole)
        if not data or data[0] == "..":
            return
        menu = QMenu(self)
        act_del = menu.addAction(tr("🗑 Verwijderen"))
        act_ren = menu.addAction(tr("✎ Hernoemen"))
        act = menu.exec(QCursor.pos())
        if act == act_del:
            self.del_remote()
        elif act == act_ren:
            self.ren_remote()


    # ---------- adresboek ----------
    def _fill_book(self, saved):
        self.cmb_book.clear()
        self.cmb_book.addItem(tr("📖 Kies verbinding..."), None)
        for e in self.book:
            if isinstance(e, dict) and e.get("host"):
                self.cmb_book.addItem(
                    f"{e.get('name') or e['host']} ({e.get('proto', 'SFTP')})",
                    e)

    def _pick_book(self, _idx=None):
        if not hasattr(self, "txt_host"):
            return  # venster wordt nog opgebouwd
        e = self.cmb_book.currentData()
        has = isinstance(e, dict)
        self.btn_save.setVisible(True)
        self.btn_delb.setVisible(has)
        if not has:
            return
        i = self.cmb_proto.findText(e.get("proto", "SFTP"))
        self.cmb_proto.setCurrentIndex(max(i, 0))
        self.txt_host.setText(e.get("host", ""))
        self.txt_port.setText(str(e.get("port", "") or ""))
        self.txt_user.setText(e.get("user", ""))
        self.txt_pass.setText(e.get("password", ""))
        i = self.cmb_mode.findText(e.get("mode", "Passief"))
        self.cmb_mode.setCurrentIndex(max(i, 0))
        self.status(tr("Verbinding '{name}' ingevuld — druk op 🔌 Verbinden.")
                    .format(name=e.get("name") or e["host"]))

    def _save_book_entry(self):
        host = self.txt_host.text().strip()
        if not host:
            QMessageBox.information(self, APP_TITLE,
                                    "Vul eerst server + gebruiker in.")
            return
        name, ok = QInputDialog.getText(
            self, APP_TITLE, "Naam voor deze verbinding:",
            text=host)
        if not ok or not name.strip():
            return
        entry = {"name": name.strip(),
                 "proto": self.cmb_proto.currentText(),
                 "host": host,
                 "port": self.txt_port.text(),
                 "user": self.txt_user.text().strip(),
                 "mode": self.cmb_mode.currentText()}
        if self.txt_pass.text():
            entry["password"] = self.txt_pass.text()
        self.book = [e for e in self.book
                     if not (isinstance(e, dict)
                             and e.get("host") == host
                             and e.get("proto") == entry["proto"])]
        self.book.append(entry)
        saved = load_remembered()
        saved["book"] = self.book
        save_remembered(saved)
        cur = entry
        self._fill_book(saved)
        idx = self.cmb_book.findData(cur)
        self.cmb_book.setCurrentIndex(idx)
        self.status(f"💾 '{name.strip()}' opgeslagen in het adresboek "
                    "(incl. wachtwoord — bestand staat in ~/.config/lopus/).")

    def _del_book_entry(self):
        e = self.cmb_book.currentData()
        if not isinstance(e, dict):
            return
        # PyQt geeft item-data als kopie terug: vergelijk op inhoud
        self.book = [x for x in self.book
                     if not (isinstance(x, dict)
                             and x.get("host") == e.get("host")
                             and x.get("proto") == e.get("proto")
                             and x.get("name") == e.get("name"))]
        saved = load_remembered()
        saved["book"] = self.book
        save_remembered(saved)
        self._fill_book(saved)
        self.status("Uit het adresboek verwijderd ✓")

    def fill_local(self):
        self.lst_local.clear()
        if os.path.dirname(self._local_path) != self._local_path:
            up = QListWidgetItem("⬆ ..")
            up.setData(Qt.ItemDataRole.UserRole,
                       (os.path.dirname(self._local_path), True))
            self.lst_local.addItem(up)
        try:
            entries = sorted(os.scandir(self._local_path),
                             key=lambda e: (not e.is_dir(), e.name.lower()))
            if not self.chk_hidden.isChecked():
                entries = [e for e in entries if not e.name.startswith(".")]
            for e in entries:
                isdir = e.is_dir()
                it = QListWidgetItem(("📁 " if isdir else "📄 ") + e.name)
                it.setData(Qt.ItemDataRole.UserRole, (e.path, isdir))
                self.lst_local.addItem(it)
        except OSError as exc:
            self.status(f"Fout: {exc}")
        self.lbl_local.setText(self._local_path)

    def local_navigate(self, item):
        path, isdir = item.data(Qt.ItemDataRole.UserRole)
        if isdir:
            self._local_path = path
            self.fill_local()

    def _start_job(self, fn, callback, busy="Bezig..."):
        """Draai netwerkwerk op de achtergrond zodat de UI niet bevriest."""
        j = getattr(self, "_job", None)
        if j is not None and j.isRunning():
            # hangende thread (bv. geblokkeerde netwerk-lees) opruimen
            j.terminate()
            j.wait(1500)
            if j.isRunning():
                self.status("Er is nog een actie bezig; even geduld...")
                return
        self._cancel_flag = False
        self._job = _Job(fn, self)
        self._job.done.connect(callback)
        self._job.prog.connect(self.bar.setValue)
        self._job.speed.connect(self.bar.setFormat)
        self._job.start()
        self.status(busy)

    def connect_now(self):
        proto = self.cmb_proto.currentText()
        host = self.txt_host.text().strip()
        if not host:
            QMessageBox.information(self, APP_TITLE,
                                    "Vul eerst een server-adres in.")
            return
        pw = self.txt_pass.text()
        if not pw:
            pw, ok = QInputDialog.getText(
                self, APP_TITLE,
                f"Wachtwoord voor {self.txt_user.text()}@{host}:",
                QLineEdit.EchoMode.Password)
            if not ok:
                return
        self.btn_conn.setEnabled(False)
        mode = self.cmb_mode.currentText()
        port = int(self.txt_port.text()) if self.txt_port.text().strip() \
            else None
        user = self.txt_user.text().strip()

        def job():
            home = self.cli.connect(proto, host, port, user, pw)
            if mode == "Actief" and self.cli.ftp:
                self.cli.ftp.set_pasv(False)
            return (home or "/").rstrip("/") or "/"

        self._start_job(
            job, lambda r: self._connect_done(r),
            busy=tr("Bezig met verbinden met {host}...").format(host=host))

    def _connect_done(self, res):
        self.btn_conn.setEnabled(True)
        proto = self.cmb_proto.currentText()
        if isinstance(res, Exception):
            QMessageBox.critical(self, APP_TITLE,
                                 f"Kan geen verbinding maken:\n{res}")
            return
        host = self.txt_host.text().strip()
        saved = load_remembered()
        saved.update({"host": host, "port": self.txt_port.text(),
                      "user": self.txt_user.text().strip(),
                      "book": getattr(self, "book", []),
                      "mode": self.cmb_mode.currentText()})
        for e in self.book:
            if (isinstance(e, dict)
                    and e.get("host") == host
                    and e.get("proto", self.cmb_proto.currentText()) == proto):
                if self.txt_pass.text():
                    e["password"] = self.txt_pass.text()
        saved["book"] = self.book
        save_remembered(saved)
        self.remote_path = res
        self.lbl_remote.setText(f"🌐 {host} — {self.remote_path}")
        for b in (self.btn_up, self.btn_down, self.btn_del_r,
                  self.btn_ren_r, self.btn_disc):
            b.setEnabled(True)
        self.btn_conn.setText(tr("✅ Verbonden"))
        self.status(tr("Verbonden ({proto}) ✓").format(proto=proto))
        self._variant = None
        self.fill_remote()

    def disconnect_now(self):
        self.cli.close()
        self.lst_remote.clear()
        self.lbl_remote.setText(tr("🌐 Server — niet verbonden"))
        self.btn_conn.setText(tr("🔌 Verbinden"))
        self.btn_conn.setEnabled(True)
        for b in (self.btn_up, self.btn_down, self.btn_del_r,
                  self.btn_ren_r, self.btn_disc):
            b.setEnabled(False)
        self.status(tr("Verbroken"))

    def fill_remote(self):
        """Laad de serverlijst op de achtergrond; bij mislukken andere
        verbindingsvariant proberen: 1) TLS-data + passief,
        2) platte data (PROT C), 3) TLS-data + actief."""
        if not self.cli.kind:
            return
        variants = [("P", False), ("C", None), ("P", True)]
        if self.cmb_mode.currentText() == "Actief":
            variants = [("P", True), ("P", False), ("C", None)]
        cur = getattr(self, "_variant", None)
        if not isinstance(cur, int) or self.cli.kind != "ftp":
            cur = None  # SFTP (of eerste keer): geen FTPS-varianten-cyclus
        order = variants[cur:] + variants[:cur] if cur is not None else variants
        path = self.remote_path
        args = getattr(self.cli, "_args", None)
        if not args:
            return

        def job():
            last = None
            for prot, active in order:
                try:
                    self.cli.close()
                    self.cli.connect(*args, no_prot_p=(prot == "C"))
                    if prot == "C":
                        self.cli.ftp.prot_c()
                    if active:
                        self.cli.ftp.set_pasv(False)
                    items = self.cli.listdir(path)
                    return items, (prot, active), None
                except Exception as e:  # noqa: BLE001
                    last = e
                    continue
            raise last or RuntimeError("geen variant werkte")

        self._start_job(job, self._fill_remote_done,
                        busy="Bezig met laden van serverlijst...")

    def _fill_remote_done(self, res):
        if isinstance(res, Exception):
            self.status(f"Fout bij lezen van map '{self.remote_path}': {res}")
            return
        items, variant, _ = res
        self._variant = variant
        prot, active = variant
        self.status(tr("Datakanaal: {prot}  —  {mode}").format(
                prot=tr("PROT C (onversleuteld)") if prot == "C" else "PROT P",
                mode=tr("actief") if active else tr("passief")))
        self.lst_remote.clear()
        up = QListWidgetItem("⬆ ..")
        up.setData(Qt.ItemDataRole.UserRole, ("..", True))
        self.lst_remote.addItem(up)
        for name, full, isdir, size in items:
            sz = human(size) if (not isdir and size) else ""
            it = QListWidgetItem(f"{'📁 ' if isdir else '📄 '}{name}"
                                 + (f"   [{sz}]" if sz else ""))
            it.setData(Qt.ItemDataRole.UserRole, (full, isdir))
            self.lst_remote.addItem(it)
        host_part = self.lbl_remote.text().split(" — ")[0] \
            if " — " in self.lbl_remote.text() else "🌐"
        self.lbl_remote.setText(f"{host_part} — {self.remote_path}")
        n_dirs = sum(1 for _n, _f, d, _s in items if d)
        self.status(tr("{n} item(s) gelezen "
                    "({d} mappen, {b} bestanden)").format(
                        n=len(items), d=n_dirs,
                        b=len(items) - n_dirs))

    def remote_navigate(self, item):
        data = item.data(Qt.ItemDataRole.UserRole)
        if not data or not self.cli.kind:
            return
        full, isdir = data[0], bool(data[1])
        if full == "..":
            self.remote_path = posixpath.dirname(
                self.remote_path.rstrip("/")) or "/"
            self.fill_remote()
        elif isdir:
            self.remote_path = full.rstrip("/") or "/"
            self._active_retry = getattr(self, "_active_retry", False)
            self.fill_remote()

    def _unique_dst(self, folder, name, remote=False):
        n = 1
        while True:
            dst = posixpath.join(folder, name) if remote \
                else os.path.join(folder, name)
            try:
                taken = self.cli.exists(dst) if remote \
                    else os.path.exists(dst)
            except Exception:  # noqa: BLE001
                return dst
            if not taken:
                return dst
            root, ext = os.path.splitext(name)
            name = f"{root}_{n}{ext}"
            n += 1

    def _cancel_transfer(self):
        self._cancel_flag = True
        self.status("⛔ Afbreken aangevraagd — stopt na de huidige chunk...")
        QTimer.singleShot(3000, self._force_kill_job)

    def _force_kill_job(self):
        j = getattr(self, "_job", None)
        if j is not None and j.isRunning():
            j.terminate()
            j.wait(1500)
            self.bar.hide()
            self.btn_cancel.hide()
            self.status("⛔ Overdracht geforceerd gestopt")
            # gedeeltelijk bestand op ruimen kan hier niet veilig meer; de
            # gebruiker kan het handmatig weggooien via het rechtermuisknop-menu

    def transfer(self, upload):
        # --- lijst van (src, dst, naam) samenstellen ---
        jobs = []
        if upload:
            for it in self.lst_local.selectedItems():
                src, isdir = it.data(Qt.ItemDataRole.UserRole) \
                    or (None, False)
                if not src or os.path.basename(src) == "..":
                    continue
                name = os.path.basename(src)
                if isdir:
                    jobs.append({"k": "dir", "src": src,
                                 "dst": posixpath.join(self.remote_path,
                                                       name), "name": name})
                else:
                    jobs.append({"k": "file", "src": src,
                                 "dst": posixpath.join(self.remote_path,
                                                       name), "name": name})
        else:
            for it in self.lst_remote.selectedItems():
                d = it.data(Qt.ItemDataRole.UserRole)
                if not d or d[0] == "..":
                    continue
                name = posixpath.basename(d[0])
                if d[1]:
                    jobs.append({"k": "dir", "src": d[0],
                                 "dst": os.path.join(self._local_path, name),
                                 "name": name})
                else:
                    jobs.append({"k": "file", "src": d[0],
                                 "dst": os.path.join(self._local_path, name),
                                 "name": name})
        if not jobs:
            self.status("Selecteer bestanden of mappen.")
            return

        # --- bestaat-al-keuze (één keer voor alle conflicterende bestanden) ---
        conflicts = []
        for j in jobs:
            if j["k"] != "file":
                continue
            d = j["dst"]
            if ((upload and self.cli.exists(d))
                    or (not upload and os.path.exists(d))):
                conflicts.append(j["name"])
        if conflicts:
            box = QMessageBox(self)
            box.setWindowTitle(APP_TITLE)
            names = "\n".join("• " + n for _s, _d, n, _sd in conflicts[:8])
            if len(conflicts) > 8:
                names += f"\n• ... en {len(conflicts) - 8} meer"
            box.setText(f"{len(conflicts)} bestand(en) bestaan al:\n{names}\n\n"
                        "Wat wil je doen?")
            ow = box.addButton("Alles overschrijven",
                               QMessageBox.ButtonRole.AcceptRole)
            rn = box.addButton("Andere naam",
                               QMessageBox.ButtonRole.ActionRole)
            box.addButton("Annuleren", QMessageBox.ButtonRole.RejectRole)
            box.exec()
            if box.clickedButton() is rn:
                jobs = [(s, (self._unique_dst(posixpath.dirname(d), n, True)
                             if side == "remote" else
                             self._unique_dst(os.path.dirname(d), n)),
                         n, side) for s, d, n, side in jobs]
            elif box.clickedButton() is not ow:
                return

        # --- achtergrondklus: alles achter elkaar, totale voortgang ---
        self._cancel_flag = False
        self.bar.show()
        self.bar.setValue(0)
        self.bar.setFormat("%p%")
        self.btn_cancel.show()
        total_files = len(jobs)

        def job():
            import time as _t
            state = {"t0": _t.time(), "last": 0, "spd": 0.0}
            done_files = 0
            total_files = sum(
                1 for j in jobs if j["k"] == "file") + sum(
                1 for j in jobs if j["k"] == "dir")
            counter = {"n": 0}

            def xfer_file(src, dst, name, upload_):
                try:
                    total = (os.path.getsize(src) if upload_
                             else (self.cli.sftp.stat(src).st_size
                                   if self.cli.kind == "sftp"
                                   else self.cli.ftp.size(src) or 0))
                except Exception:  # noqa: BLE001
                    total = 0
                state["t0"] = _t.time()
                state["last"] = 0

                def prog(pct):
                    dt = _t.time() - state["t0"]
                    if dt >= 0.5 and total > 0:
                        state["spd"] = (total * pct / 100
                                        - state["last"]) / dt
                        state["t0"] = _t.time()
                        state["last"] = total * pct / 100
                    if state["spd"] > 0:
                        self._job.speed.emit(
                            f"{counter['n'] + 1}/{total_files}: {name} — "
                            f"{human(state['spd'])}/s")
                    self._job.prog.emit(min(99, pct))

                if upload_:
                    self.cli.upload(src, dst, prog,
                                    should_stop=lambda: getattr(
                                        self, "_cancel_flag", False))
                else:
                    self.cli.download(src, dst, prog,
                                      should_stop=lambda: getattr(
                                          self, "_cancel_flag", False))

            stop = lambda: getattr(self, "_cancel_flag", False)  # noqa: E731
            errors = []
            for j in jobs:
                if stop():
                    return f"CANCELLED:{done_files}/{total_files}"
                if j["k"] == "file":
                    try:
                        xfer_file(j["src"], j["dst"], j["name"], upload)
                        done_files += 1
                    except InterruptedError:
                        return f"CANCELLED:{done_files}/{total_files}"
                    except Exception as e:  # noqa: BLE001
                        errors.append(f"{j['name']}: {e}")
                elif upload:  # map uploaden: recursief
                    for f, isd in self.cli.walk_local(j["src"]):
                        if stop():
                            return (f"CANCELLED:{done_files}/{total_files}")
                        # relatief t.o.v. de map zelf (niet de ouder!),
                        # anders komt de mapnaam dubbel in het pad
                        rel = posixpath.relpath(f, j["src"])
                        rd = posixpath.join(j["dst"], rel)
                        if isd:
                            self.cli.mkdir_p(rd)
                        else:
                            self.cli.mkdir_p(posixpath.dirname(rd))
                            try:
                                xfer_file(f, rd, posixpath.basename(f), True)
                                done_files += 1
                            except InterruptedError:
                                return (f"CANCELLED:{done_files}/"
                                        f"{total_files}")
                            except Exception as e:  # noqa: BLE001
                                errors.append(f"{rel}: {e}")
                else:  # map downloaden: recursief
                    for full, isd, _sz in self.cli.walk_remote(j["src"]):
                        if stop():
                            return (f"CANCELLED:{done_files}/{total_files}")
                        rel = posixpath.relpath(full, j["src"])
                        ld = os.path.join(j["dst"], *rel.split("/"))
                        if isd:
                            os.makedirs(ld, exist_ok=True)
                        else:
                            os.makedirs(os.path.dirname(ld), exist_ok=True)
                            try:
                                xfer_file(full, ld, posixpath.basename(full),
                                          False)
                                done_files += 1
                            except InterruptedError:
                                return (f"CANCELLED:{done_files}/"
                                        f"{total_files}")
                            except Exception as e:  # noqa: BLE001
                                errors.append(f"{rel}: {e}")
            return ("DONE", done_files, total_files, errors)

        def done(res):
            self.bar.hide()
            self.btn_cancel.hide()
            if isinstance(res, Exception):
                QMessageBox.warning(self, APP_TITLE,
                                    f"Overdracht mislukt:\n{res}")
                if upload:
                    self.fill_remote()
                else:
                    self.fill_local()
                return
            if isinstance(res, str) and res.startswith("CANCELLED"):
                self.status(f"⛔ Overdracht afgebroken ({res[10:]}) — "
                            "gedeelten verwijderd")
                if upload:
                    self.fill_remote()
                else:
                    self.fill_local()
                return
            _tag, ok, tot, errors = res
            if errors:
                QMessageBox.warning(
                    self, APP_TITLE,
                    f"{len(errors)} bestand(en) overgeslagen:\n\n"
                    + "\n".join(errors[:10])
                    + ("\n..." if len(errors) > 10 else ""))
            self.status(f"✓ {ok} bestand(en) "
                        f"{'geüpload' if upload else 'gedownload'}"
                        + (f" — ⚠ {len(errors)} overgeslagen"
                           if errors else ""))
            if upload:
                self.fill_remote()
            else:
                self.fill_local()

        self._start_job(job, done, busy=f"Bezig met "
                        f"{'uploaden' if upload else 'downloaden'} van "
                        f"{total_files} bestand(en)...")

    def del_remote(self):

        self._start_job(job, done, busy=f"Bezig met "
                        f"{'uploaden' if upload else 'downloaden'} van {name}...")

    def del_remote(self):
        it = self.lst_remote.currentItem()
        data = it.data(Qt.ItemDataRole.UserRole) if it else None
        if not data or data[0] == "..":
            return
        if QMessageBox.question(
                self, APP_TITLE,
                f"'{posixpath.basename(data[0])}' verwijderen op de server?"
        ) != QMessageBox.StandardButton.Yes:
            return
        try:
            self.cli.delete(data[0], bool(data[1]))
            self.fill_remote()
            self.status("Verwijderd ✓")
        except Exception as e:  # noqa: BLE001
            QMessageBox.warning(self, APP_TITLE, str(e))

    def ren_remote(self):
        it = self.lst_remote.currentItem()
        data = it.data(Qt.ItemDataRole.UserRole) if it else None
        if not data or data[0] == "..":
            return
        old = posixpath.basename(data[0])
        name, ok = QInputDialog.getText(self, APP_TITLE, "Nieuwe naam:",
                                        text=old)
        if not ok or not name.strip() or name.strip() == old:
            return
        try:
            self.cli.rename(data[0], name.strip())
            self.fill_remote()
            self.status("Hernoemd ✓")
        except Exception as e:  # noqa: BLE001
            QMessageBox.warning(self, APP_TITLE, str(e))

    def closeEvent(self, ev):
        self.cli.close()
        super().closeEvent(ev)


if __name__ == "__main__":  # pragma: no cover
    import sys
    app = QApplication(sys.argv)
    dlg = FtpDialog()
    dlg.exec()


