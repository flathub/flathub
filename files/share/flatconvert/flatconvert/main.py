# main.py
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

from .filemanager import FileManager
from .settings import AdwDemoWindow
from .window import FlatconvertWindow
from gi.repository import Gtk, Gio, Adw
import sys
import gi
import logging

gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')


# Logger configuration
logging.basicConfig(level=logging.INFO,
                    format='%(asctime)s - %(levelname)s - %(filename)s - %(message)s')
logger = logging.getLogger(__name__)


class FlatconvertApplication(Adw.Application):
    """The main application singleton class."""

    def __init__(self, VERSION):
        super().__init__(application_id='org.devsk.flatconvert',
                         flags=Gio.ApplicationFlags.DEFAULT_FLAGS)
        self.create_action('quit', lambda *_: self.quit(), ['<primary>q'])
        self.create_action('about', self.on_about_action)
        self.create_action(
            'preferences', self.on_preferences_action, ["<Ctrl>comma"])
        self.create_action('aide', self.on_preferences_action, ['F1'])
        self.create_action('convert', self.convert, ['c'])

        self.version = VERSION

        self.filemanager = FileManager()

    def do_activate(self):
        """Called when the application is activated.

        We raise the application's main window, creating it if
        necessary.
        """
        win = self.props.active_window
        if not win:
            win = FlatconvertWindow(application=self)

        win.present()

    def on_about_action(self, widget, _):
        """Callback for the app.about action."""
        about = Adw.AboutWindow(transient_for=self.props.active_window,
                                application_name="gnome convert",
                                application_icon='org.devsk.flatconvert',
                                developer_name='Quentin Soranzo Krebs',
                                version=self.version,
                                developers=['Quentin Soranzo Krebs',
                                            "Nathan Soranzo Krebs"],
                                copyright='© 2025 quentin')
        about.present()

    def on_preferences_action(self, widget, _):
        """Callback for the app.preferences action."""
        preference = AdwDemoWindow(transient_for=self.props.active_window,
                                   title="Preferences",
                                   default_width=600,
                                   default_height=400,
                                   resizable=True,
                                   decorated=True,
                                   modal=False,
                                   destroy_with_parent=True,
                                   hide_on_close=False)
        preference.present()

    def create_action(self, name, callback, shortcuts=None):
        """Add an application action.

        Args:
            name: the name of the action
            callback: the function to be called when the action is
              activated
            shortcuts: an optional list of accelerators
        """
        action = Gio.SimpleAction.new(name, None)
        action.connect("activate", callback)
        self.add_action(action)
        if shortcuts:
            self.set_accels_for_action(f"app.{name}", shortcuts)

    def convert():
        pass


def main(version):
    """The application's entry point."""
    app = FlatconvertApplication(version)
    return app.run(sys.argv)
