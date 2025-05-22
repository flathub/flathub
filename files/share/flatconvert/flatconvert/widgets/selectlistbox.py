# selectlistbox.py
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

from flatconvert import main
from flatconvert.widgets.select_row_box import SelectRowbox


@Gtk.Template(resource_path="/org/devsk/flatconvert/SelectListbox.ui")
class SelectListbox(Gtk.ListBox):

    __gtype_name__ = 'SelectListbox'

    def __init__(self, application):
        super().__init__()

        self.application = application

        self.filemanager = application.filemanager
        self.filemanager.connect('files-changed', self.update)

        self.delet_mode = False

        self.application.connect('toggle_delet_mode', self.set_delet_mode)
        self.application.connect('delet_clicked', self.delet_rows)

        self.connect("row-activated", self.on_row_activated)

        self.set_selection_mode(Gtk.SelectionMode.MULTIPLE)

    def update(self, file_manager, files):
        '''Update the listbox with the new files.'''

        for child in self.get_children():
            self.remove(child)
        for file in files:

            row = SelectRowbox(self, file)
            row.set_selectable(True)
            self.prepend(row)

            # self.convert_btn.set_sensitive(True)

    def get_children(self) -> tuple:
        '''Get all Gtk.ListBox children, just if it's a Gtk.ListBoxRow,
        and return a tuple.
        '''

        children = []
        child = self.get_first_child()
        # Parcours tous les enfants de la ListBox
        while child is not None:
            if isinstance(child, SelectRowbox):
                children.append(child)
            child = child.get_next_sibling()

        return children

    def set_delet_mode(self, widget, value):
        self.delet_mode = value

        for child in self.get_children():
            child.change_delet_mode(value)

        '''if self.delet_mode:
            self.addbox.set_sensitive(False)
        else:
            self.addbox.set_sensitive(True)'''

    def delet_rows(self, widget):
        files = self.filemanager.loaded_files
        widgets = self.get_children()
        for i in range(len(widgets)):
            if widgets[i].get_delet_choice():
                self.remove(widgets[i])
                self.filemanager.remove(widgets[i].input_file)

    def on_row_activated(self, listbox, row):
        pass
        '''if row.is_selected():
            self.unselect_row(row)
        elif not row.is_selected():
            self.select_row(row)'''
