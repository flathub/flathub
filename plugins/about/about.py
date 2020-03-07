#
#### Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 26th, 2020
#
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


import os
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('WebKit2', '4.0')
from gi.repository import Gtk #, WebKit2

from . import commands

class Plugin():
	
	def __init__(self, app):
		self.name = "about"
		self.app = app
		self.plugins = app.plugins_manager.plugins
		self.signal_handler = app.signal_handler
		self.commands = []
		

	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)

		
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if alt and keyval_name == "a":
			self.show_about()
	

	# def show_about(self):
	# 	w = WebKit2.WebView.new()
	# 	w.load_uri("file:///home/hamad/Documents/homepage/home.html")
	# 	#w.load_uri("https://lazka.github.io/pgi-docs/#WebKit2-4.0/classes/WebView.html#WebKit2.WebView.load_request")
	# 	
	# 	win = Gtk.Window.new(Gtk.WindowType.TOPLEVEL)
	# 	
	# 	win.add(w)
	# 	win.show_all()
		
	
	
	
	def show_about(self):
		about = Gtk.AboutDialog.new()
				       
		about.set_authors(("Hamad Al Marri", ))
		about.set_comments("lightweight text editor")
		about.set_copyright("Copyright © 2020 - Hamad Al Marri")
		about.set_documenters(("Hamad Al Marri", ))
		
		about.set_license_type(Gtk.License.GPL_3_0_ONLY)
		
		about.set_logo_icon_name("com.editor.gamma")
		about.set_program_name("Gamma Editor")
		about.set_version("0.0.1 Beta")

		about.set_website("https://gitlab.com/hamadmarri/gamma-text-editor")
		about.set_website_label("GitLab")
				       
		response = about.run()
		
		if response == Gtk.ResponseType.DELETE_EVENT:
			about.destroy()
		
