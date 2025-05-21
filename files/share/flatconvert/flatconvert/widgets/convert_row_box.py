# convert_row_box.py
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

from flatconvert.widgets.circularprogressmenu import CircularProgressMenu
from flatconvert.widgets.file_row_base import FileRowBase
from flatconvert.fileformats import SUPPORTED_FORMATS
from flatconvert.filters import filters
from flatconvert import main
from gi.repository import Adw
from gi.repository import Gtk, Gio, Pango
import gi
import os

gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')


def find_key(valeur_cible: str, dictionnaire):
    for clé, valeur in dictionnaire.items():
        if valeur == valeur_cible:
            clé_trouvée = clé
            break
    return clé


@Gtk.Template(resource_path="/com/qsk/flatconvert/ConvertRowbox.ui")
class ConvertRowbox(Gtk.ListBoxRow, FileRowBase):

    __gtype_name__ = 'ConvertRowbox'

    hbox = Gtk.Template.Child()
    file_label = Gtk.Template.Child()
    combobox_box = Gtk.Template.Child()
    input_format_label = Gtk.Template.Child()
    output_format_label = Gtk.Template.Child()
    circular_progress = Gtk.Template.Child()

    def __init__(self, application, input_file):
        super().__init__()
        FileRowBase.__init__(self, application, input_file)

        self.file_label.set_text(self.name)

        self.input_format_label.set_text(self.mime_type.split('/', 1)[1])
        self.output_format_label.set_text(self.input_file.output_format)

        self.circular_progress.set_application(application)
        self.circular_progress.set_file(input_file)

        self.input_file.connect('progress', self.on_progress)

    def update(self, file):
        self.output_format_label.set_text(self.input_file.output_format)

    def on_progress(self, widget, progress):
        if progress < 1:
            self.circular_progress.set_fraction(progress)
        elif progress == 1:
            self.circular_progress.set_fraction(progress)
            # self.progress_btn.set_icon_name("emblem-default")
