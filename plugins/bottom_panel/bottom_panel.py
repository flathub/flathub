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
#

import os
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from . import commands


class Plugin():
	
	def __init__(self, app):
		self.name = "bottom_panel"
		self.app = app
		self.signal_handler = app.signal_handler
		self.commands = []
	
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		
	
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if shift and ctrl and keyval_name == "P":
			self.show_panel()
	
	
	
	def show_panel(self):
		dir_path = os.path.dirname(os.path.realpath(__file__))
		self.builder = Gtk.Builder()
		self.builder.add_from_file(f"{dir_path}/bottom_panel.glade")
		
		window = self.builder.get_object("window")
		bottom_panel = self.builder.get_object("bottom_panel")
		
		window.remove(bottom_panel)
		bottom_panel.show()
		
		# get right side body
		right_side_body = self.app.builder.get_object("right_side_body")
		scrolled_sourceview = right_side_body.get_children()[0]
		# print(scrolled_sourceview)
		
		right_side_body.remove(scrolled_sourceview)
		
		# create paned
		paned = Gtk.Paned.new(Gtk.Orientation.VERTICAL)
		paned.pack1(scrolled_sourceview, True, False)
		paned.pack2(bottom_panel, False, True)
		
		paned.set_position(500)	
		
		right_side_body.pack_start(paned, True, True, 0)
		
		right_side_body.show_all()

	