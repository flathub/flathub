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
#	source_style: sets the style for the sourceview (text editting area)
#	"style_scheme" is set in config.py
#	style scheme for srource view style, usually sourceview style xml files are in 
#	~/.local/share/gtksourceview-4/styles (see config.py)
#	

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('GtkSource', '4')
from gi.repository import Gtk, Gdk, GtkSource

from . import source_style_commands  

class Plugin():
	
	def __init__(self, app):
		self.name = "source_style"
		self.app = app
		self.signal_handler = app.signal_handler
		self.handlers = app.signal_handler.handlers
		self.plugins = app.plugins_manager.plugins
		self.commands = []
		self.default_size  = app.config['font-size']
		self.current_size = app.config['font-size']
		self.default_font = app.config['font-family']
	
	
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		
		# the style is applied on the buffer
		source_view = self.app.sourceview_manager.source_view
		buffer = source_view.get_buffer()
		self.set_source_style(source_view)
		source_style_commands.set_commands(self)
		
		self.set_handlers()
		
		
		
	def set_handlers(self): 
		self.handlers.on_view_scroll_event = self.on_view_scroll_event
	
	
		
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if ctrl and keyval_name == "equal":
			self.update_font(1)
		elif ctrl and keyval_name == "minus":
			self.update_font(-1)
		elif ctrl and keyval_name == "0":
			self.update_font(0)
			
		
		
	def on_view_scroll_event(self, w, e):
		ctrl = (e.state & Gdk.ModifierType.CONTROL_MASK)
		
		if ctrl and e.state:
			direction = e.get_scroll_deltas()[2]
			# if scrolling down
			if direction > 0:
				self.update_font(-1)
			else:
				self.update_font(1)
				
			return True
		
	
	def update_font(self, increment):
		
		if increment == 0:
			self.current_size = self.default_size	
		
		self.current_size += increment
		size = str(self.current_size) + "px"
		files = self.plugins["files_manager.files_manager"].files
		for f in files:		
			self.update_style(f.source_view, size=size)
		
		
	def update_style(self, source_view, font=None, size=None):
	
		if not font:
			font = self.default_font
		
		if not size:
			size = str(self.default_size) + "px"

		provider = Gtk.CssProvider.new()
		css = ".sourceviewclass { font-family: '" + font + "'; font-size: " + size + "; }"
		provider.load_from_data(bytes(css, 'utf-8'))
		source_view.get_style_context().add_provider(provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

		
		
	def set_source_style(self, sourceview):
		self.update_style(sourceview)
		buffer = sourceview.get_buffer()
		
		manager = GtkSource.StyleSchemeManager.get_default()
		style = manager.get_scheme(self.app.config["style-scheme"])
		
		# the style is applied on the buffer
		buffer.set_style_scheme(style)
		
		if not buffer.get_style_scheme():
			style = manager.get_scheme("classic")
			buffer.set_style_scheme(style)
