## 
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
#	openfile: opens file(s) by showing open dialog and send filenames array
#	to files_manager.open_files method
#


import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

from .filters_mixin import FilterMixin
from . import openfile_commands as commands

class Plugin(FilterMixin):
	
	def __init__(self, app):
		self.name = "openfile"
		self.app = app
		self.signal_handler = app.signal_handler
		self.plugins = app.plugins_manager.plugins
		self.commands = []
		
		
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)
		
		
	def open_files_from_args(self, args):
		if args:
			filenames = args.split()
#			print(filenames)
			self.plugins["files_manager.files_manager"].open_files(filenames)				
		else:
			print("no GAMMA_OPEN_FILE")
		
		
	
	# key_bindings is called by SignalHandler
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		# open is bound to "<Ctrl>+o"
		if ctrl and keyval_name == "o":
			self.openfile()
			
			
			
	def openfile(self):		
		# choosefile will display the open dialog
		filenames = self.choosefile()
		# DEBUG: print(filenames)
		
		# if cancel button is pressed
		if not filenames:
			return
		
		# otherwise, let files_manager controll open, read files, and
		# set new sourceviews to each file.  
		self.plugins["files_manager.files_manager"].open_files(filenames)

	
	
	
	# show open dialog
	# (see: https://developer.gnome.org/gtk3/stable/GtkFileChooserDialog.html)
	# (see: https://developer.gnome.org/gtk3/stable/GtkFileChooser.html#GtkFileChooserAction)
	def choosefile(self):
		filenames = None
		
		# initialize file chooser 
		dialog = Gtk.FileChooserDialog("Open File", None,
										Gtk.FileChooserAction.OPEN,
										(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
										Gtk.STOCK_OPEN, Gtk.ResponseType.OK))

		# add files types filters
		self.add_filters(dialog)
		
		dialog.set_current_folder(self.plugins["files_manager.files_manager"].get_directory())
		
		# can select and open multiple files
		dialog.set_select_multiple(True)

		# show the dialog		
		response = dialog.run()
		
		if response == Gtk.ResponseType.OK:
			filenames = dialog.get_filenames()

		# close and destroy dialog object
		dialog.destroy()
		return filenames



	
		
