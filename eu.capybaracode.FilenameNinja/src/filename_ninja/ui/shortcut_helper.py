"""Platform-aware keyboard shortcut helpers.

On macOS, ``Alt`` (Option) produces special characters instead of triggering
shortcuts, so ``Alt+<key>`` bindings silently fail.  Additionally, ``Ctrl``
in Qt maps to the ``⌘`` (Command) key on macOS, which conflicts with many
system-wide shortcuts (⌘C = Copy, ⌘Q = Quit, ⌘H = Hide, ⌘M = Minimize,
etc.).

This module maps ``Alt+<key>`` shortcuts to ``Meta+<key>`` on macOS.  In Qt
terminology ``Meta`` is the **physical Control key** (``⌃``) on macOS — a
modifier that is free from system-level conflicts and commonly used by
terminal emulators and power-user apps for custom bindings.

On Windows and Linux, ``Alt+<key>`` is kept as-is.

The ``&`` mnemonic markers in :class:`QPushButton` labels (e.g.
``"&Rename"`` → ``Alt+R``) also rely on the ``Alt`` modifier and therefore
do not work on macOS.  :func:`fix_mnemonic_for_mac` patches a button so
that its mnemonic letter is reachable via ``⌃+<letter>`` on macOS.

Usage
-----
>>> from filename_ninja.ui.shortcut_helper import shortcut_key, shortcut_tooltip
>>> button.setShortcut(shortcut_key("Q"))          # Alt+Q / ⌃Q
>>> button.setToolTip(shortcut_tooltip("Open settings", "Q"))
>>>
>>> btn = QPushButton("&Rename")
>>> fix_mnemonic_for_mac(btn)   # adds ⌃R shortcut on macOS; no-op elsewhere
"""

from __future__ import annotations

import re
import sys

from PySide6.QtGui import QKeySequence, QShortcut
from PySide6.QtWidgets import QAbstractButton, QMessageBox, QWidget

# True when running on macOS.
_IS_MAC: bool = sys.platform == "darwin"

# The modifier prefix used in QKeySequence strings.
# On macOS, "Meta" maps to the physical Control key (⌃), which avoids
# conflicts with system-wide ⌘ shortcuts.
_MOD: str = "Meta" if _IS_MAC else "Alt"

# Human-readable modifier label for tooltips / UI text.
_MOD_LABEL: str = "\u2303" if _IS_MAC else "Alt"  # ⌃ on macOS

# Regex to extract the mnemonic letter from a label like "&Rename".
_MNEMONIC_RE = re.compile(r"&([A-Za-z])")


def shortcut_key(key: str) -> QKeySequence:
    """Return a :class:`QKeySequence` for *modifier + key*.

    On macOS the modifier is ``Meta`` (physical Control key, shown as ``⌃``);
    everywhere else it is ``Alt``.

    Parameters
    ----------
    key:
        A single letter or key name, e.g. ``"Q"``, ``"Z"``, ``"F5"``.
    """
    return QKeySequence(f"{_MOD}+{key}")


def shortcut_tooltip(description: str, key: str) -> str:
    """Return *description* with an appended shortcut hint.

    Example output:

    * Linux / Windows: ``"Open settings (Alt+Q)"``
    * macOS:           ``"Open settings (⌃Q)"``
    """
    if _IS_MAC:
        return f"{description} ({_MOD_LABEL}{key})"
    return f"{description} ({_MOD_LABEL}+{key})"


def fix_mnemonic_for_mac(button: QAbstractButton) -> None:
    """Make the ``&``-mnemonic shortcut work on macOS.

    On Windows / Linux this is a no-op because ``Alt+<letter>`` mnemonics
    already work.  On macOS, the function extracts the mnemonic letter from
    the button text and installs a ``Meta+<letter>`` (``⌃``) shortcut that
    triggers the button's :pyqt:`click()` signal.

    The shortcut is parented to the button so it is automatically destroyed
    when the button is deleted.
    """
    if not _IS_MAC:
        return

    text = button.text()
    m = _MNEMONIC_RE.search(text)
    if m is None:
        return

    letter = m.group(1).upper()
    sc = QShortcut(QKeySequence(f"{_MOD}+{letter}"), button)
    sc.activated.connect(button.click)


def fix_label_buddy_for_mac(
    label_text: str,
    buddy: QAbstractButton,
    parent: QWidget,
) -> None:
    """Make a ``QLabel`` buddy mnemonic work on macOS.

    When a ``QLabel`` has a ``&``-mnemonic and a buddy widget (typically a
    ``QCheckBox``), pressing ``Alt+<letter>`` activates the buddy on
    Windows / Linux.  On macOS this does nothing.

    This helper installs a ``Meta+<letter>`` (``⌃``) shortcut on *parent*
    that calls ``buddy.click()``.  It is a no-op on non-macOS platforms.

    Parameters
    ----------
    label_text:
        The raw label string **before** Qt strips the ``&`` (e.g.
        ``"&File mask case sensitive"``).
    buddy:
        The widget that should be activated (e.g. a ``QCheckBox``).
    parent:
        The widget to parent the shortcut to (usually the dialog or form
        container).
    """
    if not _IS_MAC:
        return

    m = _MNEMONIC_RE.search(label_text)
    if m is None:
        return

    letter = m.group(1).upper()
    sc = QShortcut(QKeySequence(f"{_MOD}+{letter}"), parent)
    sc.activated.connect(buddy.click)


def fix_msgbox_buttons_for_mac(box: QMessageBox) -> None:
    """Install ``⌃+<letter>`` shortcuts for every button in a QMessageBox on macOS.

    ``QMessageBox`` standard buttons (``&Yes``, ``&No``, ``&Ok``, ``&Cancel``,
    etc.) use ``Alt``-based mnemonics that do not work on macOS.  This helper
    iterates over all buttons in the message box and installs a
    ``Meta+<letter>`` (``⌃``) shortcut for each one that has a mnemonic
    marker.  It is a no-op on non-macOS platforms.
    """
    if not _IS_MAC:
        return

    for button in box.buttons():
        text = button.text()
        m = _MNEMONIC_RE.search(text)
        if m is None:
            continue
        letter = m.group(1).upper()
        sc = QShortcut(QKeySequence(f"{_MOD}+{letter}"), box)
        sc.activated.connect(button.click)
