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
# SignalHandler: is the class that manage signal handlers.
# "Handlers" is an object for mapping signal names with 
# callback methods references. "set_handlers" method is the convention
# way when need to map ui signals to callback functions. It is
# good to have the same method name in your plugin when need
# connect ui signals to your plugin's methods
#
#

import gi
from gi.repository import Gdk


class Event(object):
	pass


# "Handlers" is an object for mapping signal names with 
# callback methods references. You can mapp ui signals by
# simply handlers.on_some_ui_event = some_callback_method
class Handlers(object):
	pass
	

class SignalHandler:
	def __init__(self, app):
		self.app = app
		self.builder = app.builder
		self.plugins = app.plugins_manager.plugins
		self.handlers = Handlers()
		self.set_handlers()
		self.key_bindings_to_plugins = []
		self.any_key_press_to_plugins = []
		
	
	# SignalHandler sets the main signals such as key press 
	def set_handlers(self):
		self.handlers.on_window_key_press_event = self.on_window_key_press_event
		self.handlers.resizeBodySide = self.resizeBodySide
		self.handlers.resizeHeaderSide = self.resizeHeaderSide

		
	
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
	def on_window_key_press_event(self, window, event):
		keyval_name = Gdk.keyval_name(event.keyval)
		ctrl = (event.state & Gdk.ModifierType.CONTROL_MASK)
		alt = (event.state & Gdk.ModifierType.MOD1_MASK)
		shift = (event.state & Gdk.ModifierType.SHIFT_MASK)
		
		stop_propagation = False
		
		# for performance reason:
		# - pass only key bindings (i.e. when ctrl, alt)
		# - or when "F" function keys pressed such F1, F2 ..
		# this if is to condition the exit
		if (not ctrl and not alt) and len(keyval_name) != 2: # not F1, ..:
			for p in self.any_key_press_to_plugins:
				return_value = p.key_bindings(event, keyval_name, ctrl, alt, shift)
				if return_value:
					stop_propagation = True
		else:
			# loop through all plugins and call their key_bindings method
			# only key bindings
			for p in self.key_bindings_to_plugins:
				return_value = p.key_bindings(event, keyval_name, ctrl, alt, shift)
				if return_value:
					stop_propagation = True
		
		# print("stop_propagation", stop_propagation)
		return stop_propagation
			

	
	# when resize the left panel of the files, need
	# to resize the header too "Files"
	def resizeBodySide(self, bodyPaned, param):
		headerPaned = self.builder.get_object("headerPaned")
		headerPaned.set_position(bodyPaned.get_position())
		
	# when resize the "Files" header, need
	# to resize left panel of the files too 
	def resizeHeaderSide(self, headerPaned, param):
		bodyPaned = self.builder.get_object("bodyPaned")
		bodyPaned.set_position(headerPaned.get_position())
		
		
		
		
		
	def setup_event(self, event):
		if not hasattr(self, event):
			setattr(self, event, Event())
		
		e = getattr(self, event)
		
		if not hasattr(e, "connected"):
			e.connected = [] 
	
	
	
	def emit(self, event, data):
		self.setup_event(event)
		e = getattr(self, event)
		
		for c in e.connected:
			c(data)
	
	
	def connect(self, event, callback):
		self.setup_event(event)
		e = getattr(self, event)
		e.connected.append(callback)
	


