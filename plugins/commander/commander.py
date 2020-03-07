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


import time
import random

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GObject


from . import commands
from . import commander_window as cw
from .commands_tree import CommandsTree 


class Plugin():
	
	def __init__(self, app):
		self.name = "commander"
		self.app = app
		self.builder = app.builder
		self.plugins_manager = app.plugins_manager
		self.plugins = app.plugins_manager.plugins
		self.signal_handler = app.signal_handler
		self.handlers = app.signal_handler.handlers
		self.commands_tree = CommandsTree()
		self.commands = None
		self.only_alt = False
		self.commander_window = cw.CommanderWindow(app, self)
		
		# when user hold alt for long time
		# but never used key bindings, then
		# no need to show commander 
		# it is annoying to show commander 
		# when user hold alt but then changed
		# their mind (i.e. alt+c to copy something 
		# but changed their mind before hit the 'c'
		# when relaeas alt (without timing) commander 
		# will show which is not good
		# but with timing if alt is helf for self.max_time
		# then open commander will fire 
		self.t0 = 0
		self.max_time = 0.2 # was 0.3 
		self.cache_thread = None
		
		
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		self.signal_handler.any_key_press_to_plugins.append(self)
		self.set_handlers()
		

	def set_handlers(self):
		self.handlers.on_window_key_release_event = self.on_window_key_release_event
		self.handlers.on_commanderWindow_key_press_event = self.commander_window.on_commanderWindow_key_press_event
		self.handlers.on_commanderWindow_key_release_event = self.commander_window.on_commanderWindow_key_release_event
		self.handlers.on_commanderWindow_focus_out_event = self.commander_window.on_commanderWindow_focus_out_event
		self.handlers.on_commanderSearchEntry_changed = self.commander_window.on_commanderSearchEntry_changed
		self.handlers.on_commanderList_row_activated = self.commander_window.on_commanderList_row_activated
		self.handlers.on_commanderSearchEntry_key_press_event = self.commander_window.on_commanderSearchEntry_key_press_event
		self.handlers.on_commanderList_key_press_event = self.commander_window.on_commanderList_key_press_event
		self.handlers.on_commander_list_edge_reached = self.commander_window.on_commander_list_edge_reached
		
			

	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		# when user hit ctrl alone, or any key 
		# not ctrl + any 
		if not alt:
			# we assume that only alt is pressed
			# we know it is for sure not alt+'any key'
			# on_window_key_release_event verifies if
			# alt was released, but we need to know
			# if alt has been pressed and released (alone)
			self.only_alt = True
			
			# get time
			self.t0 = time.time()
		else:
			self.only_alt = False
			
			
	def on_window_key_release_event(self, window, event):
		keyval_name = Gdk.keyval_name(event.keyval)
		ctrl = (event.state & Gdk.ModifierType.CONTROL_MASK)
		alt = (event.state & Gdk.ModifierType.MOD1_MASK)
		shift = (event.state & Gdk.ModifierType.SHIFT_MASK)
		
		# if only alt has been pressed and released, and 
		# time is not to long during the held!, then open commander
		if alt and self.only_alt and keyval_name == "Alt_L":
			if (time.time() - self.t0) <= self.max_time:
				self.run()



	def cache_commands(self):
		# load commands only once, for first time
		# check if commands have been loaded	
		for plugin in self.plugins_manager.plugins_array:
			if plugin.commands:
				for c in plugin.commands:
					self.commands_tree.insert(c)
					
		# add self commands 
		self.commands = []
		commands.set_commands(self)
		for c in self.commands:
			self.commands_tree.insert(c)
			
		self.plugins["message_notify.message_notify"] \
							.show_message(f"{self.commands_tree.size} commands loaded")
	
	
	def add_command(self, c):
		 return self.commands_tree.insert(c)
		
		
	def remove_command(self, node):
		node = self.commands_tree.delete(node)
		return node

	
	def run(self):
		if not self.commands:
			self.cache_commands()
			
		# show commander window	
		self.commander_window.show_commander_window()
	
	
	
	
		
