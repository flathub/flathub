from __future__ import annotations

import importlib.metadata as importlib_metadata
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

import tomllib

from PySide6 import QtCore, QtGui
from PySide6.QtCore import Qt
from PySide6.QtGui import QPalette
from PySide6.QtWidgets import QApplication, QDialog, QFormLayout, QLabel, QVBoxLayout, QWidget

from .._app_metadata import APP_METADATA


def _merge_nonempty(target: dict[str, str], source: dict[str, str]) -> None:
    for key, value in source.items():
        if key in target and str(value).strip():
            target[key] = str(value)


def _parse_project_url_values(values: list[str] | None) -> dict[str, str]:
    parsed: dict[str, str] = {"homepage": "", "repository": ""}
    if not values:
        return parsed

    for item in values:
        if "," not in item:
            continue
        label, url = item.split(",", 1)
        label_norm = label.strip().lower()
        url = url.strip()
        if not url:
            continue

        if label_norm in {"homepage", "home", "website"} and not parsed["homepage"]:
            parsed["homepage"] = url
        elif label_norm in {"repository", "source", "vcs", "vcs-browser"} and not parsed["repository"]:
            parsed["repository"] = url

    return parsed


def _pkg_header(pkg: importlib_metadata.PackageMetadata, key: str) -> str:
    try:
        if key in pkg:
            return str(pkg[key]).strip()
    except Exception:
        pass
    return ""


def _read_flatpak_metainfo() -> dict[str, str]:
    """Read metadata from Flatpak AppStream metainfo, when available.

    This is a runtime fallback for Flatpak builds where Python package metadata
    fields may be stripped or unavailable.
    """

    meta: dict[str, str] = {
        "author": "",
        "license": "",
        "homepage": "",
        "repository": "",
    }

    candidates = [
        Path("/app/share/metainfo/eu.capybaracode.FilenameNinja.metainfo.xml"),
        Path("/usr/share/metainfo/eu.capybaracode.FilenameNinja.metainfo.xml"),
    ]

    xml_path = next((p for p in candidates if p.is_file()), None)
    if not xml_path:
        return meta

    try:
        root = ET.parse(xml_path).getroot()

        dev_name = root.findtext("./developer/name")
        if dev_name and dev_name.strip():
            meta["author"] = dev_name.strip()

        project_license = root.findtext("./project_license")
        if project_license and project_license.strip():
            meta["license"] = project_license.strip()

        for url_node in root.findall("./url"):
            url_type = (url_node.get("type") or "").strip().lower()
            url_value = (url_node.text or "").strip()
            if not url_value:
                continue

            if url_type == "homepage" and not meta["homepage"]:
                meta["homepage"] = url_value
            elif url_type in {"vcs-browser", "repository", "source"} and not meta["repository"]:
                meta["repository"] = url_value
    except Exception:
        return {"author": "", "license": "", "homepage": "", "repository": ""}

    return meta


def _get_project_metadata() -> dict[str, str]:
    """Read app metadata from pyproject.toml.

    Falls back to safe defaults if the file can't be read.
    """

    meta: dict[str, str] = {
        "name": "Filename Ninja",
        "version": "",
        "description": "",
        "author": "",
        "license": "",
        "homepage": "",
        "repository": "",
    }

    # 1) Primary source for distributed artifacts (Flatpak, PyInstaller, etc.).
    _merge_nonempty(meta, APP_METADATA)

    # 1b) Flatpak AppStream metainfo fallback.
    _merge_nonempty(meta, _read_flatpak_metainfo())

    # 2) Installed distribution metadata (pip/wheel install context).
    try:
        pkg = importlib_metadata.metadata("filename-ninja")
        dist_meta = {
            "name": _pkg_header(pkg, "Name"),
            "version": _pkg_header(pkg, "Version"),
            "description": _pkg_header(pkg, "Summary"),
            "author": _pkg_header(pkg, "Author"),
            "license": _pkg_header(pkg, "License"),
            "homepage": _pkg_header(pkg, "Home-page"),
            "repository": "",
        }
        project_urls = _parse_project_url_values(pkg.get_all("Project-URL"))
        dist_meta["homepage"] = dist_meta["homepage"] or project_urls["homepage"]
        dist_meta["repository"] = project_urls["repository"]
        _merge_nonempty(meta, dist_meta)
    except Exception:
        pass

    # 3) Source-tree fallback for local development runs.
    try:
        # When running from source, navigate up from:
        #   ui/about_dialog.py -> filename_ninja/ui -> filename_ninja -> src -> project root
        root = Path(__file__).resolve().parents[3]
        pyproject = root / "pyproject.toml"
        data = tomllib.loads(pyproject.read_text(encoding="utf-8"))
        proj = (data or {}).get("project", {})

        pyproject_meta: dict[str, str] = {
            "name": str(proj.get("name", "")).strip(),
            "version": str(proj.get("version", "")).strip(),
            "description": str(proj.get("description", "")).strip(),
            "author": "",
            "license": str(proj.get("license", "")).strip(),
            "homepage": "",
            "repository": "",
        }

        authors = proj.get("authors", [])
        if isinstance(authors, list) and authors:
            a0 = authors[0]
            if isinstance(a0, dict):
                pyproject_meta["author"] = str(a0.get("name", "")).strip()

        urls = proj.get("urls", {})
        if isinstance(urls, dict):
            # PEP 621 / pyproject.toml uses [project.urls] — keys are case-insensitive by convention.
            # Try capitalized first (PEP 621 style), then lowercase fallback.
            pyproject_meta["homepage"] = str(
                urls.get("Homepage", urls.get("homepage", ""))
            )
            pyproject_meta["repository"] = str(
                urls.get("Repository", urls.get("repository", ""))
            )

        _merge_nonempty(meta, pyproject_meta)
    except Exception:
        pass

    # Nicer display name.
    if meta["name"].strip().lower() in {"filename-ninja", "filename_ninja"}:
        meta["name"] = "Filename Ninja"
    return meta


class AboutDialog(QDialog):
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setWindowTitle("About")
        self.setMinimumWidth(520)
        # Non-modal: allow interacting with the main window while About is open.
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

        meta = _get_project_metadata()

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)

        # Header
        title = QLabel(f"<span style='font-size:18px; font-weight:700'>{meta['name']}</span>", self)
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setTextFormat(Qt.TextFormat.RichText)
        layout.addWidget(title)

        # Application logo (same artwork as startup splash screen).
        app_logo = QLabel(self)
        app_logo.setAlignment(Qt.AlignmentFlag.AlignCenter)
        splash_pix = QtGui.QPixmap(":/images/filename_ninja_logo.png")
        if not splash_pix.isNull():
            app_logo.setPixmap(
                splash_pix.scaled(
                    220,
                    220,
                    Qt.AspectRatioMode.KeepAspectRatio,
                    Qt.TransformationMode.SmoothTransformation,
                )
            )
            layout.addWidget(app_logo)

        version = QLabel(f"Version {meta['version']}" if meta["version"].strip() else "", self)
        version.setAlignment(Qt.AlignmentFlag.AlignCenter)
        version.setStyleSheet("color: palette(mid);")
        layout.addWidget(version)

        description = (meta.get("description") or "").strip()
        if description:
            desc_label = QLabel(description, self)
            desc_label.setWordWrap(True)
            desc_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            desc_label.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
            layout.addWidget(desc_label)

        pyside_license_notice = QLabel(
            "This application uses Qt for Python (PySide), licensed under the GNU Lesser General Public License v3.",
            self,
        )
        pyside_license_notice.setWordWrap(True)
        pyside_license_notice.setAlignment(Qt.AlignmentFlag.AlignCenter)
        pyside_license_notice.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        pyside_license_notice.setStyleSheet("color: palette(mid);")
        layout.addWidget(pyside_license_notice)

        form = QFormLayout()
        form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        form.setFormAlignment(Qt.AlignmentFlag.AlignHCenter)
        form.addRow("Version:", QLabel(meta["version"], self))
        form.addRow("Author:", QLabel(meta["author"], self))
        form.addRow("License:", QLabel(meta["license"], self))
        form.addRow("Qt:", QLabel(QtCore.qVersion(), self))
        form.addRow("Python:", QLabel(sys.version.split()[0], self))

        homepage = (meta.get("homepage") or "").strip()
        if homepage:
            homepage_label = QLabel(self)
            homepage_label.setTextFormat(Qt.TextFormat.RichText)
            homepage_label.setTextInteractionFlags(Qt.TextInteractionFlag.TextBrowserInteraction)
            homepage_label.setOpenExternalLinks(True)
            homepage_label.setText(f'<a href="{homepage}">{homepage}</a>')
            form.addRow("Website:", homepage_label)


        repo = (meta.get("repository") or "").strip()
        if repo:
            repo_label = QLabel(self)
            repo_label.setTextFormat(Qt.TextFormat.RichText)
            repo_label.setTextInteractionFlags(Qt.TextInteractionFlag.TextBrowserInteraction)
            repo_label.setOpenExternalLinks(True)
            repo_label.setText(f'<a href="{repo}">{repo}</a>')
            form.addRow("Repository:", repo_label)

        # Logo – pick the dark-theme variant when the app is in dark mode.
        logo = QLabel(self)
        logo.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)
        logo_path = (
            ":/images/capybara_code_logo_for_dark_theme.png"
            if self._is_dark_theme()
            else ":/images/capybara_code_logo.png"
        )
        pix = QtGui.QPixmap(logo_path)
        if not pix.isNull():
            logo.setPixmap(
                pix.scaled(
                    160,
                    160,
                    Qt.AspectRatioMode.KeepAspectRatio,
                    Qt.TransformationMode.SmoothTransformation,
                )
            )
        form.addRow("Published by:", logo)
        layout.addLayout(form)
        #layout.addWidget(logo)

        # No OK button: window can be dismissed via the title bar close (X).

    @staticmethod
    def _is_dark_theme() -> bool:
        """Return True when the application is currently using a dark palette."""
        app = QApplication.instance()
        if not isinstance(app, QApplication):
            return False
        pal = app.palette()
        win = pal.color(QPalette.ColorRole.Window)
        # Perceived luminance heuristic (same as theme.py).
        lum = 0.2126 * win.redF() + 0.7152 * win.greenF() + 0.0722 * win.blueF()
        return lum < 0.5
