#
#### Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Mar 4th, 2020
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
#
#
#

import os
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from . import commands

class Plugin():
	
	def __init__(self, app):
		self.name = "logger"
		self.app = app
		self.builder = app.builder
		self.signal_handler = app.signal_handler
		self.plugins = app.plugins_manager.plugins
		self.commands = []
		self.signal_handler.connect("log", self.log)
		self.signal_handler.connect("log-warning", self.log_warning)
		self.signal_handler.connect("log-error", self.log_error)
		self.log_array = []
	
	

	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)
		
	

	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if shift and ctrl and keyval_name == "L":
			self.show_log()
			
	
	
	def show_log(self, log_type=0):
		text = ""
		print("\nlog:")
		
		if log_type == 0:
			for l in self.log_array:
				text += l + '\n'
		elif log_type == 1:
			for l in self.log_array:
				if l.find("WARNING:") == 0:
					text += l + '\n'
		elif log_type == 2:
			for l in self.log_array:
				if l.find("ERROR:") == 0:
					text += l + '\n'
		
		print(text)
		self.show_log_window(text)
		
	
	
	def show_log_window(self, text):
		dir_path = os.path.dirname(os.path.realpath(__file__))
		builder = Gtk.Builder()
		builder.add_from_file(f"{dir_path}/logger.glade")
		window = builder.get_object("log_window")
		log_scrolled = builder.get_object("log_scrolled_window")
		textview = builder.get_object("log_textview")
		textview.get_buffer().set_text(text)
		window.remove(log_scrolled)
		
		style_provider = Gtk.CssProvider()
		style_provider.load_from_path(f"{dir_path}/logger.css")
		log_scrolled.get_style_context().add_provider(
			style_provider,
			Gtk.STYLE_PROVIDER_PRIORITY_USER
		)
		
		textview.get_style_context().add_provider(
			style_provider,
			Gtk.STYLE_PROVIDER_PRIORITY_USER
		)
		
		log_scrolled.show_all()
		
		# get right side body
		right_side_body = self.builder.get_object("right_side_body")
		scrolled_sourceview = right_side_body.get_children()[0]
		right_side_body.remove(scrolled_sourceview)
		
		# create paned
		paned = Gtk.Paned.new(Gtk.Orientation.VERTICAL)
		paned.pack1(scrolled_sourceview, True, False)
		paned.pack2(log_scrolled, False, True)
		paned.set_position(500)
				
		right_side_body.pack_start(paned, True, True, 0)
		right_side_body.show_all()
		
		
		
		
	
	def log(self, message):
		print(message)
		self.log_array.append(message)
		
		
	def log_warning(self, message):
		print(f'WARNING: {message}')
		self.log_array.append(f'WARNING: {message}')
		
		
	def log_error(self, message):
		print(f'ERROR: {message}')
		self.log_array.append(f'ERROR: {message}')
		
		self.plugins["message_notify.message_notify"] \
											.show_message(f'ERROR: {message}', 3)
		
		
