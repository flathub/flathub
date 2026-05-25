# shortcuts.py
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
import gettext
import locale

APP_ID = "jp.co.masatn.ImageViewer"

locale.bindtextdomain(APP_ID, "/app/share/locale")
locale.textdomain(APP_ID)

gettext.bindtextdomain(APP_ID, "/app/share/locale")
gettext.textdomain(APP_ID)

gettext.install(APP_ID)


import gi

gi.require_version("Gtk", "4.0")

from gi.repository import Gtk


@Gtk.Template(resource_path="/jp/co/masatn/ImageViewer/shortcuts.ui")
class ImageviewerShortcuts(Gtk.ShortcutsWindow):
    __gtype_name__ = "ImageviewerShortcuts"

    def __init__(self, parent):
        super().__init__()

        self.set_transient_for(parent)
