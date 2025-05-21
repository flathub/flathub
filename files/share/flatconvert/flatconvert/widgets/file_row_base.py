# file_row_base.py
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


class FileRowBase():
    def __init__(self, application, input_file):

        self.input_file = input_file
        self.path = input_file.input_dir_path
        self.input_path = input_file.input_dir_path
        self.output_path = input_file.output_dir_path
        self.name = input_file.name
        self.output_format = self.input_file.output_format
        self.mime_type = input_file.mime_type

        self.input_file.connect('changed', self.update)

    def update(self, file):
        self.path = file.input_dir_path
        self.input_path = file.input_dir_path
        self.output_path = file.output_dir_path
        self.name = file.name
        self.output_format = self.input_file.output_format
