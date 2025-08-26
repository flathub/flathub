# main.py
#
# Copyright 2025 Addison Matto'law
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

import sys
import gi
import os
import subprocess
import time
from rich.console import Console
from rich.progress import Progress
from rich.panel import Panel
from rich.text import Text

gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')

from gi.repository import Gtk, Gio, Adw
from .window import OhthatremindsmeWindow

console = Console()

class OhthatremindsmeApplication(Adw.Application):
    """The main application singleton class."""

    def __init__(self):
        super().__init__(application_id='com.ohthatremindsme.downloader',
                         flags=Gio.ApplicationFlags.DEFAULT_FLAGS,
                         resource_base_path='/com/ohthatremindsme/downloader')
        self.create_action('quit', lambda *_: self.quit(), ['<primary>q'])
        self.create_action('about', self.on_about_action)
        self.create_action('preferences', self.on_preferences_action)

    def do_activate(self):
        """Called when the application is activated."""
        win = self.props.active_window
        if not win:
            win = OhthatremindsmeWindow(application=self)
        win.present()

    def on_about_action(self, *args):
        """Callback for the app.about action."""
        about = Adw.AboutDialog(application_name='OhThatRemindsMe Archive Downloader',
                                application_icon='com.ohthatremindsme.downloader',
                                developer_name='Addison Matto\'law',
                                version='0.1.0',
                                developers=['Addison Matto\'law'],
                                copyright='© 2025 Addison Matto\'law')
        about.set_translator_credits(_('translator-credits'))
        about.present(self.props.active_window)

    def on_preferences_action(self, widget, _):
        try:
            console.print("Installing dependencies...", style="bold yellow")
            subprocess.run(['sudo', 'dnf', 'install', '-y', 'python3-pip'], check=True)
            subprocess.run(['pip', 'install', 'feedparser', 'rich'], check=True)
            console.print("Dependencies installed successfully!", style="bold green")
        except subprocess.CalledProcessError as e:
            console.print(f"[red]An error occurred while installing dependencies: {e}[/red]")
            sys.exit(1)

    def create_action(self, name, callback, shortcuts=None):
        """Add an application action."""
        action = Gio.SimpleAction.new(name, None)
        action.connect("activate", callback)
        self.add_action(action)
        if shortcuts:
            self.set_accels_for_action(f"app.{name}", shortcuts)

def display_metadata():
    """Display metadata from the feed."""
    console.print(Panel("Displaying Metadata:", title="Metadata", title_align="left"))
    subprocess.run(['python3', 'otrm.py', 'https://www.ohthatremindsme.com/feed.xml'])

def create_music_directory():
    """Create a directory in the user's Music folder."""
    music_dir = os.path.expanduser("~/Music/OhThatRemindsMe")
    os.makedirs(music_dir, exist_ok=True)
    return music_dir

def run_podcast_archiver(music_dir):
    """Run the podcast archiver command."""
    console.print(f"Archiving podcasts to [cyan]{music_dir}[/cyan]...", style="bold yellow")
    with Progress() as progress:
        task = progress.add_task("[cyan]Archiving! \n\n This may take a while...", total=100)
        subprocess.run(['podcast-archiver', '--dir', music_dir, '--feed', 'https://www.ohthatremindsme.com/feed.xml'])
        while not progress.finished:
            progress.update(task, advance=1)
            time.sleep(0.1)  # Simulate work being done

def main(argv):
    console.print("OhThatRemindsMe Downloader!", style="bold magenta")
    
    app = OhthatremindsmeApplication()
    app.on_preferences_action(None, None)  # Call preferences action
    display_metadata()
    
    time.sleep(2)  # Sleep for 2 seconds
    music_dir = create_music_directory()
    run_podcast_archiver(music_dir)

if __name__ == "__main__":
    main(sys.argv[1:])
