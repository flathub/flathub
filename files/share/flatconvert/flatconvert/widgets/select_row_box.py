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

from gi.repository import Adw
from gi.repository import Gtk, Gio, Pango
import os

from flatconvert import main
from flatconvert.filters import filters
from flatconvert.fileformats import SUPPORTED_FORMATS
from flatconvert.widgets.file_row_base import FileRowBase
from flatconvert.utils import find_key
from flatconvert.widgets.setting_menu import SettingMenu


@Gtk.Template(resource_path="/com/qsk/flatconvert/SelectRowbox.ui")
class SelectRowbox(Gtk.ListBoxRow, FileRowBase):

    __gtype_name__ = 'SelectRowbox'

    hbox = Gtk.Template.Child()
    file_label = Gtk.Template.Child()
    combobox_box = Gtk.Template.Child()
    input_format_label = Gtk.Template.Child()
    output_format_combobox = Gtk.Template.Child()
    popover_button = Gtk.Template.Child()
    chekbtn_select = Gtk.Template.Child()
    select_btn_revealer = Gtk.Template.Child()

    def __init__(self, application, input_file):
        super().__init__()
        FileRowBase.__init__(self, application, input_file)

        self.filemanager = application.filemanager

        self.setting_menu = SettingMenu(self.input_file, self.filemanager)
        self.popover_button.set_popover(self.setting_menu)

        self.file_label.set_text(self.name)

        self.input_format_label.set_text(self.mime_type.split('/', 1)[1])

        self.connect("activate", self.on_row_activated)

        # add the output file formats to the combobox
        for format in SUPPORTED_FORMATS.get(self.mime_type.split('/', 1)[0]):
            self.output_format_combobox.append_text(format)

        self.output_format_combobox.set_active(
            SUPPORTED_FORMATS[
                self.mime_type.split('/', 1)[0]]
            .index(self.output_format))

    @Gtk.Template.Callback()
    def output_format_changed(self, combobox):
        self.input_file.set_output_format(combobox.get_active_text())

    def change_delet_mode(self, mode):  # ajouter un e à  delet
        if mode:
            self.select_btn_revealer.set_reveal_child(mode)
        else:
            self.select_btn_revealer.set_reveal_child(mode)

    def get_delet_choice(self):
        return self.chekbtn_select.get_active()

    def on_row_activated(self, row):
        pass
