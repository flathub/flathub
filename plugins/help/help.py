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
import subprocess
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('WebKit2', '4.0')
from gi.repository import Gtk #, WebKit2

from . import commands

class Plugin():
	
	def __init__(self, app):
		self.name = "help"
		self.app = app
		self.plugins = app.plugins_manager.plugins
		self.signal_handler = app.signal_handler
		self.commands = []
		

	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)

		
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if keyval_name == "F1":
			self.show_help()
	

	
	
	def show_help(self):
		dir_path = os.path.dirname(os.path.realpath(__file__))
		subprocess.Popen(["yelp", f"{dir_path}/index.page"])
		
