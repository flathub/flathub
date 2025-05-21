# open_row_box.py
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

from gi.repository import Adw
from gi.repository import Gtk, Gio, Pango
import os
from flatconvert import main
from flatconvert.filters import filters
from flatconvert.fileformats import SUPPORTED_FORMATS
from flatconvert.widgets.file_row_base import FileRowBase
from flatconvert.utils import open_file_manager


def find_key(valeur_cible: str, dictionnaire):
    for clé, valeur in dictionnaire.items():
        if valeur == valeur_cible:
            clé_trouvée = clé
            break
    return clé


@Gtk.Template(resource_path="/com/qsk/flatconvert/OpenRowbox.ui")
class OpenRowbox(Gtk.ListBoxRow, FileRowBase):

    __gtype_name__ = 'OpenRowbox'

    hbox = Gtk.Template.Child()
    file_label = Gtk.Template.Child()

    def __init__(self, application, input_file):
        super().__init__()
        FileRowBase.__init__(self, application, input_file)

        self.file_label.set_text(
            f"{self.name.split('.')[0]}.{self.input_file.output_format}")

    def update(self, file):
        super().update(file)
        self.file_label.set_text(
            f"{self.name.split('.')[0]}.{self.input_file.output_format}")

    @Gtk.Template.Callback()
    def open_filemanager(self, button):
        open_file_manager(self.output_path)
