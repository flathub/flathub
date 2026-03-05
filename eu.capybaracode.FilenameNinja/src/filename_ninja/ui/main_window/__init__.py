"""Main window package.

This package contains the split-out implementation of the main application window.

The legacy module [`filename_ninja.ui.main_window`](../main_window.py:1) re-exports
[`FilenameNinjaApp`](window.py:1) for backward compatibility.
"""

from .window import FilenameNinjaApp

__all__ = ["FilenameNinjaApp"]
