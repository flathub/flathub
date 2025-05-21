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
from flatconvert.widgets.open_listbox import OpenListbox


@Gtk.Template(resource_path="/com/qsk/flatconvert/OpenPage.ui")
class OpenPage(Gtk.Box):

    __gtype_name__ = 'OpenPage'

    output_files_container = Gtk.Template.Child()
    done_btn = Gtk.Template.Child()

    def __init__(self, application, **kargs):
        super().__init__(**kargs)

        self.application = application

        self.openlistbox = OpenListbox(self.application)
        self.output_files_container.append(self.openlistbox)

        self.done_btn.connect("clicked", self.application.reset)