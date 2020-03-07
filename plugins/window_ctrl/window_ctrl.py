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
# window_ctrl: is responsible for handling basic window operations
# - Maximize, minimize, quit
#

from . import commands

class Plugin():
	
	def __init__(self, app):
		self.name = "window_ctrl"
		self.app = app
		self.signal_handler = app.signal_handler
		self.builder = app.builder
		self.window = app.window
		self.plugins = app.plugins_manager.plugins
		self.handlers = app.signal_handler.handlers
		self.is_maximized = None
		
		# commands and set_commands are important for
		# the commander plugin to know this plugin methods, key bindings, description
		self.commands = []
		
		self.set_handlers()
		self.message_notify = None
		self.openfile = None
		
	
	
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)
		
		self.N = self.builder.get_object("new_menu")
		self.O = self.builder.get_object("open_menu")
		self.D = self.builder.get_object("project_menu")
		self.S = self.builder.get_object("save_menu")
		self.F = self.builder.get_object("find_menu")
		self.R = self.builder.get_object("find_replace_menu")
		self.W = self.builder.get_object("welcome_menu")
		self.H = self.builder.get_object("help_menu")
		self.A = self.builder.get_object("about_menu")
		
		
	
	
	# setting handlers, see SignalHandler
	def set_handlers(self):
		self.handlers.on_closeBtn_release_event = self.on_closeBtn_release_event
		self.handlers.on_maximizeBtn_release_event = self.on_maximizeBtn_release_event
		self.handlers.on_minimizeBtn_release_event = self.on_minimizeBtn_release_event
		
		self.handlers.on_new_menu_button_press_event = self.on_new_menu_button_press_event
		self.handlers.on_open_menu_button_press_event = self.on_open_menu_button_press_event
		self.handlers.on_project_menu_button_press_event = self.on_project_menu_button_press_event
		self.handlers.on_save_menu_button_press_event = self.on_save_menu_button_press_event
		self.handlers.on_find_menu_button_press_event = self.on_find_menu_button_press_event
		self.handlers.on_find_replace_menu_button_press_event = self.on_find_replace_menu_button_press_event
		self.handlers.on_welcome_menu_button_press_event = self.on_welcome_menu_button_press_event
		self.handlers.on_help_menu_button_press_event = self.on_help_menu_button_press_event
		self.handlers.on_about_menu_button_press_event = self.on_about_menu_button_press_event
		

	
	# this method got called by SignalHandler. Use it wisely
	# if your plugin spend much time in this method it will delay
	# other plugins when calling their key_bindings method
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if alt and ctrl and keyval_name == "m":
			self.minimize()
		elif alt and keyval_name == "m":
			self.toggle_maximize()
		elif ctrl and keyval_name == "q":
			self.quit()
			

	
	# iconify is the gtk method to minimize window
	def minimize(self):
		self.window.iconify()
		
		
		
	def toggle_maximize(self):
		if self.window.is_maximized():
			self.window.unmaximize()
		else:
			self.window.maximize()
			
			
	def quit(self):
		# before quit, need to stop any notify message
		# because of the thread sleep in message_notify
		# must cancel the thread		
		self.plugins["message_notify.message_notify"].cancel()
		
		# close all files
		self.plugins["files_manager.files_manager"].close_all()
		
		# if all files are closed (user didn't click "don't close")
		editted_counter = self.plugins["files_manager.files_manager"].editted_counter
		if editted_counter == 0:
			self.app.quit()
		else:
			print(f"!!!! edited {editted_counter}")
		
		
	def on_closeBtn_release_event(self, widget, event):
		self.quit()
	
	def on_maximizeBtn_release_event(self, widget, event):
		self.toggle_maximize()
	
	def on_minimizeBtn_release_event(self, widget, event):
		self.minimize()
	
	
	
	####################### MENU ##############################
	def on_new_menu_button_press_event(self, widget, event):
		self.plugins["files_manager.files_manager"].create_new_file()

			
			
	################ OPEN #######################
	def on_open_menu_button_press_event(self, widget, event):
		self.plugins["files_manager.openfile"].openfile()
		
	################ NEW #########################
	def on_project_menu_button_press_event(self, widget, event):
		self.plugins["files_manager.opendir"].opendir()
	
	################ SAVE ########################
	def on_save_menu_button_press_event(self, widget, event):
		self.plugins["files_manager.savefile"].save_all()
		
			
			
		
	def on_find_menu_button_press_event(self, widget, event):
		self.plugins["find_and_replace.find_and_replace"].show_window(show_replace=False)
		
	def on_find_replace_menu_button_press_event(self, widget, event):
		self.plugins["find_and_replace.find_and_replace"].show_window(show_replace=True)
		
	def on_welcome_menu_button_press_event(self, widget, event):
		self.plugins["welcome.welcome"].show_welcome()
		
	def on_help_menu_button_press_event(self, widget, event):
		self.plugins["help.help"].show_help()
		
	def on_about_menu_button_press_event(self, widget, event):
		self.plugins["about.about"].show_about()
		
		
	
	
	
	####################### css control #########################
	def grap_attention(self, menu=None):
		if not menu:
			menu = self.H
			
		# add "sourceviewclass" css class
		menu.get_style_context().add_class("menu_attention")
		
		
		
	def remove_attention(self, menu=None):
		if not menu:
			menu = self.H
			
		# add "sourceviewclass" css class
		menu.get_style_context().remove_class("menu_attention")
		
		
