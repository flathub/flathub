# filemanager.py
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

import os
import mimetypes
import queue
import threading
import logging
import gi
gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
from gi.repository import Gtk, Gio, GLib, GObject

from flatconvert.file import File
from flatconvert.filters import filters
from flatconvert.convert_thread import ConvertThread

logger = logging.getLogger(__name__)

# TODO : Remove useless methods


class FileManager(GObject.GObject):
    """
    FileManager is a singleton class that manages file operations, including loading,
    removing, resetting, and converting files. It also provides signals for UI updates
    and progress tracking during file conversions.
    Attributes:
        __gsignals__ (dict): Defines custom GObject signals for file changes,
            file conversions, and conversion progress.
        _instance (FileManager): Singleton instance of the FileManager class.
        loaded_files (list): List of loaded files.
        convert_queue (queue.Queue): FIFO queue for file conversions.
        progress_callback (callable): Callback function for updating the UI.
        _progress_callback (callable): Internal callback for progress communication.
        conversion_thread (threading.Thread): Thread for handling file conversions.
        is_running (bool): Indicates whether a conversion process is running.
        converted_count (int): Number of files converted so far.
        total_files (int): Total number of files to be converted.
        convert_progress (float): Progress of the current conversion process.
    Methods:
        get_instance(cls, value=None):
            Returns the singleton instance of FileManager.
        remove(file):
            Removes a file from the loaded files list and emits a 'files-changed' signal.
        reset():
            Resets the FileManager state, clearing loaded files and conversion queue.
        convert_all(callback=None):
            Starts the conversion process for all loaded files in a separate thread.
        set_progress_callback(callback):
            Sets the callback function for updating the UI during conversion.
        progress_communication(progress=None):
            Communicates the conversion progress to the UI.
        get_format_with_name(file_name) -> str:
            Returns the file format (extension) of the given file name.
        get_type_with_name(file_name) -> str:
            Returns the MIME type category (e.g., 'image', 'video') of the given file name.
        get_files() -> list:
            Returns the list of loaded files.
        add_file(file):
            Adds a file to the loaded files list and emits a 'files-changed' signal.
        _update_signal():
            Emits the 'files-changed' signal.
        _converted_signal():
            Emits the 'files-converted' signal.
        load_response(dialog, response):
            Handles the response from a file chooser dialog and adds the selected file.
        open_file_dialog(function, action=Gtk.FileChooserAction.OPEN, application=None, current_name=None):
            Opens a file chooser dialog with the specified parameters.
    """


    __gsignals__ = {
        'files-changed': (GObject.SignalFlags.RUN_FIRST, None, (GObject.TYPE_PYOBJECT,)),
        'files-converted': (GObject.SignalFlags.RUN_FIRST, None, ()),
        'convert-progress': (GObject.SignalFlags.RUN_FIRST, None, (float,))
    }

    _instance = None

    def __new__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance = super(FileManager, cls).__new__(cls)
            # Initialisation de votre FileManager
        return cls._instance

    def __init__(self):
        super().__init__()

        if getattr(self, "_initialized", False):
            return
        FileManager._instance = self  # Ensure the singleton instance is properly initialized
        self.loaded_files = []
        self.convert_queue = queue.Queue()
        self.progress_callback = None  # Callback pour l'UI
        self.conversion_thread = None
        self.is_running = False
        self.converted_count = None
        self.total_files = 0  # Nombre total de fichiers à convertir
        self.convert_progress = None

    @classmethod
    def get_instance(cls):
        """ Return the singleton instance of FileManager. """
        if cls._instance is None:
            cls()
        return cls._instance

    def remove(self, file):
        """ Remove a file from the loaded files list. """
        self.loaded_files.remove(file)
        self._update_signal()

    def reset(self) -> None:
        """ Reset the FileManager state. """
        self.loaded_files = []
        self.convert_queue = queue.Queue()  # File FIFO pour les conversions
        self.progress_callback = None  # Callback pour l'UI
        self.conversion_thread = None
        self.is_running = False
        self.converted_count = None
        self.total_files = 0  # Nombre total de fichiers à convertir

        self._update_signal()

    def convert_all(self, callback=None):
        """ Start the conversion process for all loaded files.
        This method initializes the conversion thread and starts the
        conversion process.
        It also sets the total number of files to be converted and
        initializes the converted count.
        """

        def thread_finished_callback():
            self.is_running = False

        self.convert_process = ConvertThread(self._progress_communication)
        self.conversion_thread = threading.Thread(
            target=lambda: (self.convert_process.start_conversion(self.loaded_files), thread_finished_callback()),
            daemon=True
        )
        self.conversion_thread.start()
        self.is_running = True

    def _progress_communication(self, total_files : int, completed_files : int, file_pct : float, file : File):
        self.emit('convert-progress' , completed_files + file_pct)
        file.set_progress(file_pct)
        if completed_files == total_files:
            self._converted_signal()

    def terminate_conversion(self, _):
        """ Terminate the conversion process if it is running. """
        if self.is_running:
            self.convert_process.terminate()
            self.is_running = False
            logger.info("Conversion terminated")
            self._converted_signal()

    def get_format_with_name(
        self, file_name) -> str: return file_name.split('.', 1)[1]

    def get_type_with_name(self, file_name) -> str:
        try:
            return self.get_mime_type(file_name).split('/', 1)[0]
        except Exception:
            return ""

    def get_files(self) -> list: return self.loaded_files

    def add_file(self, file):
        """Ajoute un fichier à la liste chargée."""
        self.loaded_files.append(file)
        logger.info("file added : %s", file.name)
        self._update_signal()

    def _update_signal(self): self.emit('files-changed', self.loaded_files)
    def _converted_signal(self): self.emit('files-converted')

    def load_response(self, dialog, response):
        """ Handles the response from a file chooser dialog.
        If the user accepts, it retrieves the selected file and adds it to
        the loaded files."""
        if response == Gtk.ResponseType.ACCEPT:
            active_filter = dialog.get_filter().get_name()

            file = dialog.get_file()
            path = file.get_path()

            NewFile = File(file)

            self.add_file(NewFile)

    def open_file_dialog(
        self,
        function,
        action=Gtk.FileChooserAction.OPEN,
        application=None,
        current_name=None
    ):

        self._native = Gtk.FileChooserNative(
            title="Save File",
            action=action,
            # accept_label="_Save",
            cancel_label="_(_Cancel)",
        )

        if action == Gtk.FileChooserAction.SAVE and current_name:
            self._native.set_current_name(current_name)

        filters(self._native)

        self._native.connect("response", function)
        self._native.show()
