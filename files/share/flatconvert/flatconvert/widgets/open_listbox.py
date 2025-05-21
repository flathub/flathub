# open_listbox.py
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

from .open_row_box import OpenRowbox
from flatconvert.filters import filters
from flatconvert import main
from gi.repository import Adw
from gi.repository import Gtk, Gio, Pango
import gi

gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')


@Gtk.Template(resource_path="/com/qsk/flatconvert/OpenListbox.ui")
class OpenListbox(Gtk.ListBox):

    __gtype_name__ = 'OpenListbox'

    def __init__(self, application):
        super().__init__()

        self.application = application

        self.filemanager = application.filemanager
        self.filemanager.connect('files-changed', self._update)

        '''gesture = Gtk.GestureClick()
        gesture.connect("pressed", self.fonction)
        self.addbox.add_controller(gesture)'''

    def _update(self, _, files):
        """Update the UI with the filemanager file list."""
        for child in self.get_children():
            self.remove(child)

        for file in files:
            row = OpenRowbox(self, file)
            self.prepend(row)

    def get_children(self) -> tuple:
        '''Get all Gtk.ListBox children, just if it's a Gtk.ListBoxRow,
        and return a tuple.
        '''

        children = []
        child = self.get_first_child()
        # Parcours tous les enfants de la ListBox
        while child is not None:
            if isinstance(child, OpenRowbox):  # Vérifie si c'est un Gtk.Label
                children.append(child)
            child = child.get_next_sibling()

        return children
