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
#
#	files_manager: is responsible to manage all opened documents.
#
#
#

import os
from pathlib import Path

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from . import files_manager_commands as commands

from .file import File
from .create_file_mixin import CreateFileMixin
from .close_file_mixin import CloseFileMixin
from .open_file_mixin import OpenFileMixin
from .commands_ctrl import CommandsCtrl


class Plugin(CommandsCtrl, CreateFileMixin, CloseFileMixin, OpenFileMixin):
	
	def __init__(self, app):
		self.name = "files_manager"
		self.app = app
		self.signal_handler = app.signal_handler
		self.builder = app.builder
		self.plugins = app.plugins_manager.plugins
		self.sourceview_manager = app.sourceview_manager
		self.commands = []
		self.files = []
		self.current_file = None
		self.current_directory = str(Path.home())
		self.counter = 1
		self.editted_counter = 0
		
	
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		
		commands.set_commands(self)
		
		# default empty file when open editor with no opened files
		self.current_file = File(self, "empty", self.builder.get_object("view"), new_file=True, init_file=True)

		# add empty/current_file to files array
		self.files.append(self.current_file)
		
		self.signal_handler.emit("file-switched", self.current_file.source_view)
				
	
	# key_bindings is called by SignalHandler
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):		
		# close current file is bound to "<Ctrl>+w"
		if ctrl and keyval_name == "w":
			# close current_file
			self.close_current_file()
		elif shift and ctrl and keyval_name == "W":
			self.close_all()
		elif ctrl and keyval_name == "n":
			self.create_new_file()
			
		elif shift and ctrl and keyval_name == "Z":
			print("files\n")
			for i, f in enumerate(self.files):
				print(i, f.filename)
			
	
	
	def rename_file(self, file_object, filename):
		# check if it is the new init file, need to make new sourceview and be added to ui
		if file_object.init_file:
			self.duplicate_init_file(file_object, filename)
			
		# if new file added by the user
		else:
			# rename in array
			
			# remove old command in commander
			self.update_commanders_remove(file_object)
			
			# rename file
			file_object.filename = filename
			
			# add new commander for the file
			self.update_commanders_add(file_object)
						
			file_object.new_file = False	# not new anymore
			self.plugins["ui_manager.ui_manager"].rename_file(file_object)
			
		
		
	
	def duplicate_init_file(self, file_object, filename):
		newsource = self.sourceview_manager.get_new_sourceview()
			
		# default empty file when open editor with no opened files
		newfile = File(self, filename, newsource)
		
		# copy text from init file to newfile
		buffer = file_object.source_view.get_buffer()
		text = buffer.get_text(buffer.get_start_iter(), buffer.get_end_iter(), True)
		newsource.get_buffer().set_text(text)
		file_object.source_view.get_buffer().set_text("")

		# add newfile to files array
		self.add_file_to_list(newfile)
		
		self.plugins["ui_manager.ui_manager"].add_filename_to_ui(newfile)
		self.switch_to_file(len(self.files) - 1)		
				
				
	
	# handler of "clicked" event
	# it switch the view to the filename in clicked button
	def side_file_clicked(self, filename):
		# is_already_openned gets the index of the file in "files" array
		file_index = self.is_already_openned(filename)
		
		# if found, which should!, switch to it
		if file_index >= 0:
			self.switch_to_file(file_index) 
	
	
	
	
	
	def switch_to_file(self, file_index):
	
		if file_index < 0:
			return
	
		# check if it is the current_file, then exit method 
		if self.current_file == self.files[file_index]:
			return
		
		buffer = self.current_file.source_view.get_buffer()
		self.plugins["highlight.highlight"].remove_highlight(buffer)
		
		# get file object
		f = self.files[file_index]
				
		# replace the source view
		self.plugins["ui_manager.ui_manager"].replace_sourceview_widget(f.source_view)
		
		self.current_file = f
		
		# update ui, set selected
		self.plugins["ui_manager.ui_manager"].set_currently_displayed(self.current_file.ui_ref)
			
		# update headerbar to filename
		self.plugins["ui_manager.ui_manager"].update_header(f.filename, f.editted)
		
		# show message of the full path of the file 
		# it is useful to avoid confusion when having 
		# different files with similar names in different paths
		self.plugins["message_notify.message_notify"].show_message(f.filename)
		
		
		self.signal_handler.emit("file-switched", self.current_file.source_view)

		
		
	# returns file index if found or -1
	def is_already_openned(self, filename):
		return self.get_file_index(filename)
		
		
		
		
	def get_file_index(self, filename):
		for i, f in enumerate(self.files):
			if filename == f.filename:
				return i	
		return -1
		
	
	
	def add_file_to_list(self, newfile):
		self.files.append(newfile)
		self.update_commanders_add(newfile)
		
		
	def remove_file_from_list(self, file_object, file_index):
		self.update_commanders_remove(file_object)
		del self.files[file_index]


	def get_directory(self):
		if self.current_file.parent_dir:
			return self.current_file.parent_dir
		
		return self.current_directory
		