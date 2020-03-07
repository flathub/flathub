#
#### Author: first lastname <email>
#### Date: MMM ddth, yyyy
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
# template: is not used plugin, but it is an example or template for you to 
#			start your plugin. It is good starting point to your plugin.
#			Copy this file, rename it, change self.name to your plugin name.
#			If the plugin needs to export its commands to commander, copy
#			commands.py too. If not, delete lines:
#									from . import commands
#									commands.set_commands(self)
# 			
#			Currently, activate method must be implemented.
#			If no implemention needed for activate then keep
#			the "pass"
#
#			The usual imports are:
#			import gi
#			gi.require_version('Gtk', '3.0')
#			gi.require_version('GtkSource', '4')
#			from gi.repository import GLib, Gio, Gtk, Gdk, GtkSource, GObject
#			
#			But your plugin may not need all of these modules
#
#

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from . import commands

# class name must be "Plugin". Do not change the name 
class Plugin():
	
	# the plugins_manager will pass "app" reference 
	# to your plugin. "app" object is defined in gamma.py
	# from "app" reference you can access pretty much 
	# everything related to Gamma (i.e. window, builder, 
	# sourceview, and other plugins)
	def __init__(self, app):
		self.name = "template"
		self.app = app
		self.signal_handler = app.signal_handler # optional
		self.handlers = app.signal_handler.handlers # optional
		self.commands = []
	
	# do not remove 
	def activate(self):
		# self.signal_handler.key_bindings_to_plugins.append(self) <-- if need key_bindings
		commands.set_commands(self)
		self.set_handlers() # optional
		pass
	
	# works with "self.signal_handler.key_bindings_to_plugins.append(self)"
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		# you should not map "on_window_key_press_event" to your plugin.
		# this function will help you by getting the keyval_name("e", "space", ..)
		# and other modifiers like ctrl, alt, and shift 
		# Simply uncomment: 
		# 		self.signal_handler.key_bindings_to_plugins.append(self)   or
		#		self.signal_handler.any_key_press_to_plugins.append(self)
		# and then check what key binding you need such as
		# if alt and ctrl and keyval_name == "m":
		# 	...
		# the above "if" is checking whether alt and ctrl are hold when
		# pressed the "m" key (i.e. <Ctrl><Alt>+m)
		#
		# key_bindings_to_plugins vs any_key_press_to_plugins
		# key_bindings_to_plugins for key bindings only (<Ctrl>+a)
		# However any_key_press_to_plugins will call your key_bindings
		# method for any key press! Sometimes is needed but usually
		# for shortcuts use key_bindings_to_plugins
		pass
	
	
	# optional
	# setting handlers, see SignalHandler
	def set_handlers(self):
		# self.handlers.on_closeBtn_release_event = self.on_closeBtn_release_event
		pass
		
		
	# optional	
	def method1(self):
		pass
		
	# optional
	def method2(self):
		pass
	
	# optional
	def method3(self):
		pass
