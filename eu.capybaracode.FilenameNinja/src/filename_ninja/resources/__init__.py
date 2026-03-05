"""Application packaged resources (Qt .qrc and generated resource module).

This package exists so UI code can reliably import [`resources_rc`](resources/resources_rc.py:1)
for side effects (Qt resource registration) regardless of how the project is executed.
"""

from __future__ import annotations

from PySide6.QtCore import QFile, QIODevice


def load_resource_text(resource_path: str) -> str:
    """Read a UTF-8 text file from the compiled Qt resource system.

    Parameters
    ----------
    resource_path:
        A Qt resource path such as ``":/html/help.html"``.

    Returns
    -------
    str
        The file contents, or an empty string if the resource could not be read.
    """
    f = QFile(resource_path)
    if f.open(QIODevice.OpenModeFlag.ReadOnly):
        data = f.readAll().data()
        f.close()
        return bytes(data).decode("utf-8")
    return ""
