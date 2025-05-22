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
from flatconvert.widgets.convertlistbox import ConvertListbox


@Gtk.Template(resource_path="/org/devsk/flatconvert/ConvertPage.ui")
class ConvertPage(Gtk.Box):

    __gtype_name__ = 'ConvertPage'

    convertbox_contain = Gtk.Template.Child()
    progress_bar = Gtk.Template.Child()
    cancel_btn = Gtk.Template.Child()
    next_btn = Gtk.Template.Child()

    def __init__(self, application, **kargs):
        super().__init__(**kargs)

        self.application = application

        self.filemanager = application.filemanager
        self.filemanager.connect('files-converted', self._files_converted)
        self.filemanager.connect('convert-progress', self._convert_progress)

        self.convertlistbox = ConvertListbox(self.application)
        self.convertbox_contain.append(self.convertlistbox)

        self.cancel_btn.connect("clicked", self.filemanager.terminate_conversion)
        self.next_btn.connect("clicked", self.page1)

    def page1(self, button):  # TODO : rename
        self.application.stack.set_visible_child_name("open_files_page")

    def _files_converted(self, button):
        self.cancel_btn.set_visible(False)
        self.next_btn.set_visible(True)

    def _convert_progress(self, _, progress):
        self.progress_bar.set_fraction(progress)


    # TODO : Set progress_bar progress when a conversion is in progress.
