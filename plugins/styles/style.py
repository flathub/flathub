#### Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 11th, 2020
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#
#
#
#	style: sets the theme style to Gamma window
#	the style.css file is set in config.py "style-path"
#
#
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

class Plugin():
	
	def __init__(self, app):
		self.name = "style"
		self.app = app
		self.commands = []
		
	
	def activate(self):
		style_provider = Gtk.CssProvider()
		style_provider.load_from_path(self.app.config["style-path"])
		Gtk.StyleContext.add_provider_for_screen(
			Gdk.Screen.get_default(), style_provider,
			Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
		)
		