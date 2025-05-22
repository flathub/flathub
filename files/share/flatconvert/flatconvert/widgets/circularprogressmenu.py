# circularprogressmenu.py
#
# Copyright 2024-2025 Quentin Soranzo Krebs
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

from flatconvert.utils import *
from gi.repository import Adw
from gi.repository import Gtk, GObject
import logging

from .circularprogress import CircularProgress


@Gtk.Template(resource_path="/org/devsk/flatconvert/CircularProgressMenu.ui")
class CircularProgressMenu(Gtk.MenuButton, CircularProgress):
    """
    CircularProgressMenu is a custom widget that combines the functionality
    of a Gtk.MenuButton and a CircularProgress. It is designed to display a circular progress indicator and provide
    menu functionality.
    Attributes:
        __gtype_name__ (str): The GType name for the widget.
        row_progressbar (Gtk.Template.Child): A child widget representing the progress bar.
    Methods:
        __init__(*args, **kargs):
            Initializes the CircularProgressMenu widget.
        do_snapshot(snapshot):
            Captures the visual representation of the widget for rendering.
        set_fraction(fraction):
            Sets the progress fraction for both the CircularProgress and the row_progressbar.
        cancel_conversion(_):
            Callback function to cancel an ongoing file conversion process.
        set_application(application):
            Sets the application instance to access the file manager.
        set_file(file):
            Associates a file object with the widget.
    """

    __gtype_name__ = 'CircularProgressMenu'

    row_progressbar = Gtk.Template.Child()

    def __init__(self, *args, **kargs):
        super().__init__(*args, **kargs)

        self._filemanager = None
        self.file = None

    def do_snapshot(self, snapshot):
        """
        Captures the visual representation of the widget and its children.
        This method is responsible for taking a snapshot of the widget's
        current state and rendering it into the provided `snapshot` object.
        It combines the snapshot functionality of both `Gtk.MenuButton` and
        `CircularProgress` to ensure the widget's appearance is accurately
        captured.
        Args:
            snapshot (Gtk.Snapshot): The snapshot object used to render the
                widget's visual representation.
        """
        
        Gtk.MenuButton.do_snapshot(self, snapshot)
        CircularProgress.do_snapshot(self, snapshot)

    def set_fraction(self, fraction):
        """
        Sets the progress fraction for both the CircularProgress and the
        row_progressbar. This method updates the visual representation of
        the progress indicator to reflect the current progress value.
        Args:
            fraction (float): The progress fraction to set, typically a value
                between 0.0 and 1.0.
        """

        CircularProgress.set_fraction(self, fraction)
        self.row_progressbar.set_fraction(fraction)

    @Gtk.Template.Callback()
    def cancel_conversion(self, _): #TODO: make it work
        try:
            self.file.kill_conversion_worker
        except Exception as e:
            logger.error(f"Error while killing conversion worker: {e}")

    def set_application(self, application):
        """
        Sets the application instance to access the file manager.
        This method allows the widget to interact with the application's
        file manager for file-related operations.
        Args:
            application (Application): The application instance containing
                the file manager.
        """

        self._filemanager = application.filemanager

    def set_file(self, file):
        """
        Associates a file object with the widget. This method allows the
        widget to display progress and interact with the specified file.
        Args:
            file (File): The file object associated with the widget.
        """

        self.file = file


GObject.type_register(CircularProgressMenu)
