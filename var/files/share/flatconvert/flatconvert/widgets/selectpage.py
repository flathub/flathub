# selectpage.py
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
from gi.repository import Gtk
from flatconvert.widgets.selectlistbox import SelectListbox


@Gtk.Template(resource_path="/com/qsk/flatconvert/SelectPage.ui")
class SelectPage(Gtk.Box):

    __gtype_name__ = 'SelectPage'

    listbox_contain = Gtk.Template.Child()
    convert_btn = Gtk.Template.Child()
    main_contain = Gtk.Template.Child()
    add_btn = Gtk.Template.Child()

    def __init__(self, application, **kargs):
        super().__init__(**kargs)

        self.application = application
        self.filemanager = application.filemanager
        self.filemanager.connect('files-changed', self._files_changed)

        self.selectlistbox = SelectListbox(self.application)
        self.listbox_contain.prepend(self.selectlistbox)
        self.selectlistbox.set_selection_mode(Gtk.SelectionMode.MULTIPLE)

        self.add_btn.connect(
            "clicked",
            lambda *args: self.filemanager.open_file_dialog(
                self.filemanager.load_response,
                Gtk.FileChooserAction.OPEN
            )
        )

    @Gtk.Template.Callback()
    def convert(self, button):
        self.application.stack.set_visible_child_name("page2")

        self.application.filemanager.convert_all()


    def _files_changed(self, _, files):
        if len(files) == 0:
            self.convert_btn.set_sensitive(False)
        else:
            self.convert_btn.set_sensitive(True)