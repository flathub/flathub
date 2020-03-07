#
#### Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 11th, 2020
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
# This is the entry point to Gamma editor. The gamma.py will load
# config parameters and load the builder (UI structure of the main window).
# This Application instance is the main root of gamma. It holds
# references to everything needed for other plugins such as config, 
# window, builder, sourceview_manager, and plugins_manager.
# Also it loads the eager plugins in self.plugins_manager.load_plugins()
# which call activate for each plugin and store plugins references in
# plugins_manager.plugins


import os

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('GtkSource', "4")
from gi.repository import GLib, Gio, Gtk, Gdk, GtkSource, GObject

import config
import sourceview_manager
import signal_handler
from plugins.plugins_manager import PluginsManager

class Application(Gtk.Application):

	def __init__(self, *args, **kwargs):	
		
		# make the package name as "com.editor.gamma"
		# FLAGS_NONE means no passing arguments from command line, this
		# might be changed later to support new window, new file, or open a file
		super().__init__(*args, application_id="com.editor.gamma", 
						flags=Gio.ApplicationFlags.FLAGS_NONE, **kwargs)
		
		# this line is important to mak gtk object(newer version of pygtk) to
		# include gtk sourceview.
		GObject.type_register(GtkSource.View)
		
		self.window = None
		
		# config contains important paths and settings for ui, styles, plugins
		self.config = config.config_paths_and_settings

		# builder is the object responsible of
		# translating .ui xml files (widgets design/layout. see glade) to
		# be in the gtk objects form 
		self.load_builder()
		
		# plugins_manager for anything related to plugins (eager plugins)
		self.plugins_manager = PluginsManager(self)
				
		# signal_handler is for handling general signals such as
		# key press, and basic window resizing paned
		# SignalHandler also makes it easier for other plugins to
		# process key bindings. It loop through all plugins and 
		# call key_bindings function passing (event, keyval_name, ctrl, alt, shift)
		# which is an easy design for plugins to set there key bindings
		self.signal_handler = signal_handler.SignalHandler(self)
		
		# sourceview_manager for anything related to sourceview
		self.sourceview_manager = sourceview_manager.SourceViewManager(self)


	def load_builder(self):
		self.builder = Gtk.Builder()
		
		# load .ui file, its path is in config file
		self.builder.add_from_file(self.config["ui-path"])



	def set_handlers(self):
		# this line connects signals in handlers object to 
		# some functions. "handlers" is set by SignalHandler and
		# plugins that need to bind signals to functions
		self.builder.connect_signals(self.signal_handler.handlers)
		

	def do_startup(self):
		Gtk.Application.do_startup(self)


	def do_activate(self):
		if not self.window:
			# get id=window (ui element in .ui) from builder
			self.window = self.builder.get_object("window")
			
			# must set the parent application of 
			# window to this app(self)
			self.window.props.application = self
		
		
		# loading plugins calls their activate functions.
		# in plugins_manager.py, you can comment out plugins in
		# plugin_list array
		self.plugins_manager.load_plugins()
		self.set_handlers()
		
		filenames = os.getenv('GAMMA_OPEN_FILE')
		self.plugins_manager.plugins["files_manager.openfile"].open_files_from_args(filenames)
		
		# self.window.maximize()
		self.window.set_icon_name("com.editor.gamma")
		# self.window.set_icon_name("/home/hamad/dev/pygtk/gamma/icon.svg")
		self.window.show_all()



if __name__ == "__main__":
	app = Application()
	app.run()
	
