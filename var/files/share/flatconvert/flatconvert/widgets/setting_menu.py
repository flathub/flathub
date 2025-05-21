# setting_menu.py
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
from gi.repository import Gtk, GObject
import os
import logging

from flatconvert import main

logger = logging.getLogger(__name__)


@Gtk.Template(resource_path="/com/qsk/flatconvert/SettingMenu.ui")
class SettingMenu(Gtk.Popover):

    __gtype_name__ = 'SettingMenu'

    input_file_label = Gtk.Template.Child()
    output_file_entry = Gtk.Template.Child()
    preview_image = Gtk.Template.Child()
    input_row = Gtk.Template.Child()
    output_row = Gtk.Template.Child()

    def __init__(self, file, filemanager, **kargs):
        super().__init__(**kargs)

        self.filemanager = filemanager
        self.file = file
        self.input_path = file.input_full_path
        self.output_path = file.output_full_path
        self.file_name = file.name
        self.input_file_label.set_text(self.input_path)
        self.output_file_entry.set_text(self.output_path)
        self.input_row.set_tooltip_text(self.output_path)
        self.output_row.set_tooltip_text(self.output_path)
        self.preview_image.set_from_pixbuf(file.thumbnail)

        self.file.connect("changed", self._on_file_changed)
        self.output_file_entry.connect("changed", self._on_entry_changed)

    def _on_popover_button_clicked(self, button):
        # Vérifie que le popover est attaché à un parent valide
        if self.get_parent() is not None:
            self.popup()
        else:
            logger.warning("The popover don't have valid parent.")

    def _on_close_button_clicked(self, button):
        self.close()

    @Gtk.Template.Callback()
    def _open_file_chooser(self, button, _):
        self.filemanager.open_file_dialog(
            self._file_chooser_load_response,
            Gtk.FileChooserAction.SAVE,
            current_name=self.file_name)

    def _file_chooser_load_response(self, dialog, response):
        if response == Gtk.ResponseType.ACCEPT:
            active_filter = dialog.get_filter().get_name

            file = dialog.get_file()
            full_path = file.get_path()

            self.output_file_entry.set_text(full_path)

    def _change_output_path(self, new_path):
        self.file.set_output_path(new_path)

    def _on_file_changed(self, file):
        self.input_path = file.input_full_path
        self.output_path = file.output_full_path
        self.file_name = file.name
        self.input_file_label.set_text(self.input_path)
        self.input_row.set_tooltip_text(self.output_path)
        self.output_row.set_tooltip_text(self.output_path)
        self.preview_image.set_from_pixbuf(file.thumbnail)

    def _on_entry_changed(self, _):
        self._change_output_path(self.output_file_entry.get_text())


GObject.type_register(SettingMenu)
