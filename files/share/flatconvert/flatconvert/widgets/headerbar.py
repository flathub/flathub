# headerbar.py
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
from gi.repository import Gtk, Gio
from flatconvert import main


@Gtk.Template(resource_path="/org/devsk/flatconvert/HeaderBar.ui")
class HeaderBar(Adw.Bin):

    __gtype_name__ = 'HeaderBar'

    previous_btn = Gtk.Template.Child()
    _headerbar = Gtk.Template.Child()
    toggle_delet = Gtk.Template.Child()
    select_options = Gtk.Template.Child()
    trash_btn = Gtk.Template.Child()

    def __init__(self, application):
        super().__init__()

        self.application = application
        self.application.connect('toggle_delet_mode', self.delet_mode)

        self.application.connect("page_changed", self._on_page_changed)

    def delet_mode(self, widget, value):
        if value:
            self.trash_btn.set_visible(True)
        else:
            self.trash_btn.set_visible(False)

    def _on_page_changed(self, _, page_name):
        if page_name == "SelectPage":
            self.select_options.set_visible(True)
        else:
            self.select_options.set_visible(False)
