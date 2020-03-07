#
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
#	opendir: shows open folder dialog and opens all files 
#	under the selected folder (recursively) and send filenames array 
#	to files_manager.open_files method
#

import os
from urllib.parse import urlparse

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

from . import opendir_commands as commands

class Plugin():
	
	def __init__(self, app):
		self.name = "opendir"
		self.app = app
		self.signal_handler = app.signal_handler
		self.plugins = app.plugins_manager.plugins
		self.commands = []
		
		
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)
		
			
	
	# key_bindings is called by SignalHandler
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		
		if ctrl and shift and keyval_name == "O":
			self.opendir()
			
			
			
	def opendir(self):
		filenames = []
		
		# choosefile will display the open dialog
		dir_path = self.chooseDir()
		# print(dir_path)
		
		# if cancel button is pressed
		if not dir_path:
			return
		
		# otherwise, let files_manager controll open, read files, and
		# set new sourceviews to each file.
		for root, dirs, files in os.walk(dir_path, topdown=False):
			for filename in files:
				(name, ext) = os.path.splitext(filename)
				if name[0] != '.' and ext != ".pyc":
					#print(os.path.join(root, filename))
					filenames.append(os.path.join(root, filename))
		    	
		    
		
		
		if filenames:
			self.plugins["files_manager.files_manager"].open_files(filenames)
		    

	
	
	
	# show open dialog
	# (see: https://developer.gnome.org/gtk3/stable/GtkFileChooserDialog.html)
	# (see: https://developer.gnome.org/gtk3/stable/GtkFileChooser.html#GtkFileChooserAction)
	def chooseDir(self):
		final_path = None
		
		# initialize file chooser 
		dialog = Gtk.FileChooserDialog("Open Directory", None,
										Gtk.FileChooserAction.SELECT_FOLDER,
										(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
										Gtk.STOCK_OPEN, Gtk.ResponseType.OK))
		
		dialog.set_current_folder(self.plugins["files_manager.files_manager"].get_directory())
		
		# can select and open multiple files
		# dialog.set_select_multiple(True)

		# show the dialog		
		response = dialog.run()
		
		if response == Gtk.ResponseType.OK:
			dir_path = dialog.get_uri()
			p = urlparse(dir_path)
			final_path = os.path.abspath(os.path.join(p.netloc, p.path))
		# elif response == Gtk.ResponseType.CANCEL:
		#	print("Cancel clicked")

		# close and destroy dialog object
		dialog.destroy()
		return final_path


		
