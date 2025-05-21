# window.py
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

from flatconvert.utils import *
from gi.repository import Adw
from gi.repository import Gtk, Gio, Pango, GLib, GObject
import threading
import logging

from flatconvert import main
from .widgets.headerbar import HeaderBar
from .widgets.convertlistbox import ConvertListbox
from .widgets.open_listbox import OpenListbox
from .widgets.selectpage import SelectPage
from .widgets.convertpage import ConvertPage
from .widgets.openpage import OpenPage

# Initialisation des threads
GObject.threads_init()

logger = logging.getLogger(__name__)


@Gtk.Template(resource_path='/com/qsk/flatconvert/window.ui')
class FlatconvertWindow(Adw.ApplicationWindow):

    __gtype_name__ = 'FlatconvertWindow'

    __gsignals__ = {
        'toggle_delet_mode': (GObject.SignalFlags.RUN_FIRST, bool, (bool,)),
        'delet_clicked': (GObject.SignalFlags.RUN_FIRST, bool, ()),
        'page_changed': (GObject.SignalFlags.RUN_FIRST, bool, (str,))
    }

    stack = Gtk.Template.Child()
    toolbar_view = Gtk.Template.Child()

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.filemanager = kwargs.get('application').filemanager

        self.selectpage = SelectPage(self)
        self.stack.add_titled(self.selectpage, "page1", "selection")

        self.convertpage = ConvertPage(self)
        self.stack.add_titled(self.convertpage, "page2", "conversion")

        self.openpage = OpenPage(self)
        self.stack.add_titled(
            self.openpage, "open_files_page", "open_files_page")

        # Connecter le signal pour détecter le changement de page
        self.stack.connect('notify::visible-child', self.on_page_changed)

        self._headerbar = HeaderBar(self)
        self.toolbar_view.add_top_bar(self._headerbar)
        self._headerbar.previous_btn.connect("clicked", self.previous_page)
        self._headerbar.toggle_delet.connect("toggled", self.toggle_delet_mode)
        self._headerbar.trash_btn.connect(
            "clicked", self.on_trash_button_clicked)

    def previous_page(self, button):
        pages = self.stack.get_pages()  # Liste des pages
        current_page = self.stack.get_visible_child()  # Page actuelle
        for i, page in enumerate(pages):
            if page.get_child() == current_page and i - 1 < len(pages):
                self.stack.set_visible_child(
                    pages[i - 1].get_child())  # Aller à la suivante
                break

    def go_to_page(self, index):
        # Obtenir la liste des enfants du Gtk.Stack
        children = self.stack.get_pages()

        # Vérifier si l'index est valide
        if 0 <= index < len(children):
            # Obtenir le nom de la page correspondante
            page_name = self.stack.get_child_by_name(
                children[index].get_name())
            # Naviguer vers la page
            self.stack.set_visible_child(page_name)
        else:
            logger.error("Index out of range")

    def next_page(self, button):
        pages = self.stack.get_pages()  # Liste des pages
        current_page = self.stack.get_visible_child()  # Page actuelle
        for i, page in enumerate(pages):
            if page.get_child() == current_page and i + 1 < len(pages):
                self.stack.set_visible_child(
                    pages[i + 1].get_child())  # Aller à la suivante
                break

    def on_conversion_progress(self, progress):
        """Callback appelé après chaque fichier converti."""
        self.convertpage.progress_bar.set_fraction(progress)

    def toggle_delet_mode(self, button):
        self.emit('toggle_delet_mode', button.get_active())

    def on_trash_button_clicked(self, button):
        self.emit('delet_clicked')
        self._headerbar.toggle_delet.set_active(False)
        self.emit('toggle_delet_mode',
                  self._headerbar.toggle_delet.get_active())

    def reset(self, button):
        self.filemanager.reset()
        self.go_to_page(0)

    def get_active_page(self):
        if self.stack:
            visible_child = self.stack.get_visible_child()
            if visible_child:
                return visible_child.get_name
        return None

    def on_page_changed(self, stack, _):
        visible_child = stack.get_visible_child()
        if visible_child:
            self.emit('page_changed', visible_child.get_name())
            logger.info("Page change to : %s", visible_child.get_name())
