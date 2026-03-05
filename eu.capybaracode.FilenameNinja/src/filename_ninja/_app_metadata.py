"""Application metadata embedded in the package.

This module is the primary runtime metadata source for distributed builds
(Flatpak, PyInstaller executables, etc.) where ``pyproject.toml`` is not
available next to the installed application files.
"""

from __future__ import annotations

APP_METADATA: dict[str, str] = {
    "name": "Filename Ninja",
    "version": "1.0.0",
    "description": "Multi-platform open source GUI application for bulk renaming of files.",
    "author": "Zbyněk Šťáva",
    "license": "MIT",
    "homepage": "https://capybara-code.gitlab.io/filename-ninja/",
    "repository": "https://gitlab.com/capybara-code/filename-ninja/",
}
