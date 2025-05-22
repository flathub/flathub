# file.py
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
import io
import subprocess
import tempfile
from PIL import Image
import mimetypes
import uuid
import logging
import concurrent.futures
from flatconvert.fileformats import SUPPORTED_FORMATS
from flatconvert.utils import *
import gi

gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
gi.require_version("GdkPixbuf", "2.0")
from gi.repository import Adw, Gtk, Gio, GObject, GLib, GdkPixbuf

logger = logging.getLogger(__name__)


class File(GObject.GObject):
    """
    File Class
    This class represents a file object and provides functionality for managing
    file paths, formats, and conversion processes. It is designed to work with
    Gio.File objects and supports serialization and deserialization for
    persistence.
    Attributes:
        id (str): A unique identifier for the file object.
        file (Gio.File): The Gio.File object associated with this file.
        input_full_path (str): The full path of the input file.
        input_path (str): The directory path of the input file.
        input_name (str): The name of the input file.
        mime_type (str): The MIME type of the input file.
        media_type (str): The media type of the input file (e.g., 'image', 'text').
        input_format (str): The format of the input file (e.g., 'png', 'txt').
        output_format (str): The desired output format for the file conversion.
        output_path (str): The directory path for the output file.
        output_full_path (str): The full path of the output file.
        output_name (str): The name of the output file.
        progress (float): The progress of the file conversion process (0.0 to 1.0).
        conversion_worker (Optional[Process]): The worker process handling the
            file conversion.
    Signals:
        'changed': Emitted when the file's state changes.
        'progress': Emitted when the conversion progress updates.
    Methods:
        __init__(gio_file: Gio.File):
            Initializes the File object with a Gio.File instance.
        _init_paths():
            Initializes the input and output paths and formats.
        _update_output_full_path():
            Updates the full path of the output file based on the current settings.
        __getstate__():
            Returns a dictionary of simple attributes for pickling.
        __setstate__(state):
            Reconstructs the object from pickled data.
        set_output_format(format: str) -> None:
            Sets the output format for the file conversion process.
        set_progress(progress: float) -> None:
            Updates the conversion progress and emits the 'progress' signal.
        set_output_path(new_path: str) -> None:
            Sets a new output path for the file and updates related attributes.
        kill_conversion_worker() -> None:
            Terminates the conversion worker process if it is running.
    Properties:
        input_full_path (str): Gets or sets the full path of the input file.
        output_full_path (str): Gets or sets the full path of the output file.
        input_dir_path (str): Gets the directory path of the input file.
        output_dir_path (str): Gets the directory path of the output file.
        mime_type (str): Gets or sets the MIME type of the input file.
        input_format (str): Gets or sets the format of the input file.
        output_format (str): Gets or sets the desired output format.
        name (str): Gets or sets the name of the input file.
        output_name (str): Gets or sets the name of the output file.
        conversion_worker (Optional[Process]): Gets or sets the conversion worker
        process.
        """

    __gsignals__ = {
        'changed': (GObject.SignalFlags.RUN_FIRST, None, ()),
        'progress': (GObject.SignalFlags.RUN_FIRST, None, (float,))
    }

    def __init__(self, gio_file: Gio.File):
        super().__init__()
        self.id = uuid.uuid4().hex
        self.file = gio_file
        self._init_paths()
        self.progress = 0.0
        self.conversion_worker = None
        self._changed_callbacks = []
        self._progress_callbacks = []

    def _init_paths(self):
        self.input_full_path = self.file.get_path()
        self.input_path = os.path.dirname(self.input_full_path)
        self.input_name = os.path.basename(self.input_full_path)
        self.mime_type, _ = mimetypes.guess_type(self.input_full_path)
        self.media_type, self.input_format = self.mime_type.split('/', 1)
        self.output_format = self.input_format
        self.output_path = self.input_path
        self._update_output_full_path()
        self.thumbnail = self._generate_thumbnail(self)

    @staticmethod
    def _generate_thumbnail(file):
        """ Generate the file thumbnail. """
        try:
            path = file.input_full_path
            mime_main = file.mime_type.split('/', 1)[0]
            if mime_main == "image":
                image = Image.open(path)
                img_bytes = io.BytesIO()
                image.save(img_bytes, format='PNG')
                img_bytes.seek(0)

            elif mime_main == "video":
                # Use ffmpeg to extract a frame from the video at a specific time (e.g., 5 seconds)
                with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp_file:
                    ffmpeg_command = [
                        "ffmpeg",
                        "-y",
                        "-i", path,
                        "-ss", "00:00:05",  # Extract frame at 5 seconds
                        "-frames:v", "1",
                        "-update", "1",
                        tmp_file.name
                    ]
                    subprocess.run(ffmpeg_command, check=True)

                    # Load the generated thumbnail image into memory
                    with open(tmp_file.name, "rb") as img_file:
                        img_data = img_file.read()

                # Use Pillow to open the image from memory
                img = Image.open(io.BytesIO(img_data))

                # Convert the image to a format compatible with Gtk.Image
                img_bytes = io.BytesIO()
                img.save(img_bytes, format="PNG")
                img_bytes.seek(0)

            else:
                logger.error(f"Mime_main {mime_main} not recognized!")

            # Create a GdkPixbuf from the image data
            return GdkPixbuf.Pixbuf.new_from_stream(Gio.MemoryInputStream.new_from_bytes(GLib.Bytes.new(img_bytes.getvalue())))

        except Exception as e:
            logger.error(f"Thumbnail generation error in {file.path}: {e}")

    def _update_output_full_path(self):
        base, _ = os.path.splitext(self.input_name)
        self.output_full_path = os.path.join(
            self.output_path,
            f"{base}.{self.output_format}"
        )
        self.output_name = os.path.basename(self.output_full_path)

    def __getstate__(self):
        # Retourne les attributs simples pour pickle
        return {
            'input_full_path': self.input_full_path,
            'output_format': self.output_format,
            'output_path': self.output_path,
            'id': self.id
        }

    def __setstate__(self, state):
        # Reconstruit l'objet avec les données picklées
        from gi.repository import Gio
        gio_file = Gio.File.new_for_path(state['input_full_path'])
        self.__init__(gio_file)
        self.output_format = state['output_format']
        self.output_path = state['output_path']
        self._update_output_full_path()
        self.id = state['id']

    # Propriétés pour les attributs
    @property
    def input_full_path(self):
        return self._input_full_path

    @input_full_path.setter
    def input_full_path(self, value):
        self._input_full_path = value

    @property
    def output_full_path(self):
        return self._output_full_path

    @output_full_path.setter
    def output_full_path(self, value):
        self._output_full_path = value

    @property
    def input_dir_path(self):
        return self.input_path

    @property
    def output_dir_path(self):
        return self.output_path

    @property
    def mime_type(self):
        return self._mime_type

    @mime_type.setter
    def mime_type(self, value):
        self._mime_type = value

    @property
    def input_format(self):
        return self._input_format

    @input_format.setter
    def input_format(self, value):
        self._input_format = value

    @property
    def output_format(self):
        return self._output_format

    @output_format.setter
    def output_format(self, value):
        self._output_format = value

    @property
    def name(self):
        return self.input_name

    @name.setter
    def name(self, value):
        self.input_name = value

    @property
    def output_name(self):
        return self._output_name

    @output_name.setter
    def output_name(self, value):
        self._output_name = value

    @property
    def conversion_worker(self):
        return self._conversion_worker

    @conversion_worker.setter
    def conversion_worker(self, value):
        self._conversion_worker = value

    def set_output_format(self, format: str) -> None:
        """
        Set the output format for the file conversion process.

        This function configures the desired output format for files that are
        being converted.
        It ensures that the resulting files will adhere to the specified
        format, which can be
        useful for standardizing file outputs across different processes or
        systems.

        Parameters:
        - format (str): A string representing the desired output format
            (e.g., 'gif', 'txt', 'jpeg').

        Raises:
        - ValueError: If the specified format is not supported.

        Example:
        ```python
        set_output_format('jpeg')
        ```
        """
        if format in SUPPORTED_FORMATS[self.media_type]:
            self.output_format = format
            self.output_full_path = os.path.join(
                self.output_path,
                f"{os.path.splitext(self.input_name)[0]}.{self.output_format}"
            )
            self.emit('changed')
        else:
            raise ValueError(
                f"Unsupported format: {format}. "
                f"Supported formats for {self.media_type} are: "
                f"{', '.join(SUPPORTED_FORMATS[self.media_type])}."
            )

    def set_progress(self, progress: float) -> None:
        self.progress = progress
        self.emit('progress', progress)

    def set_output_path(self, new_path) -> None:
        self.output_path = os.path.dirname(new_path)  # only directory path
        self.output_full_path = new_path
        self.output_name = os.path.basename(new_path)
        self.input_name = os.path.basename(new_path)
        self.emit('changed')

    def kill_conversion_worker(self):
        if self.conversion_worker:
            self.conversion_worker.terminate()
            self.conversion_worker = None
