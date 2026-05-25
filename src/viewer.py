# viewer.py
#
# Copyright 2026 masatn
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later
import os
import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")

from gi.repository import Gtk, Adw


@Gtk.Template(resource_path="/jp/co/masatn/ImageViewer/viewer.ui")
class ImageViewerDialog(Adw.Window):
    __gtype_name__ = "ImageViewerDialog"

    picture = Gtk.Template.Child()

    def __init__(self, parent):
        super().__init__(transient_for=parent)

        shortcut = Gtk.Shortcut.new(
            Gtk.ShortcutTrigger.parse_string("Escape"),
            Gtk.NamedAction.new("window.close"),
        )

        self.add_shortcut(shortcut)

    def open_image(self, path):
        self.set_title(os.path.basename(path))
        self.picture.set_filename(path)
