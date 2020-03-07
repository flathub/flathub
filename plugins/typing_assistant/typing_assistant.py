
#### Author: Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 27th, 2020
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
#
#	Auto types closing brackets and quotes while typing.
#
#


class Plugin():
	
	def __init__(self, app):
		self.name = "typing_assistant"
		
		self.app = app
		self.plugins = app.plugins_manager.plugins
		self.signal_handler = app.signal_handler 
		self.commands = []
		self.chars = {
			"quotedbl": "\"", 
			"apostrophe": "'",
			"parenleft": "(",
			"bracketleft": "[",
			"braceleft": "{",
			 "less": "<" 
		}
		self.close = {
			"\"": "\"",
			"'": "'",
			"(": ")",
			"[": "]",
			"{": "}",
			"<": ">",
		}


	def activate(self):
		self.signal_handler.any_key_press_to_plugins.append(self)
		self.signal_handler.key_bindings_to_plugins.append(self)

	
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if keyval_name in self.chars:
			return self.text_insert(keyval_name)
		elif ctrl and keyval_name == "Return":
			self.move_to_next_line()
		
		
		
	def move_to_next_line(self):		
		# get current viewing file' buffer
		buffer = self.plugins["files_manager.files_manager"].current_file.source_view.get_buffer()
		
		# get selection bound
		selection = buffer.get_selection_bounds()
		
		# if selected text, exit
		if selection != ():
			return False
		
		position = buffer.get_iter_at_mark(buffer.get_insert())
		position.forward_to_line_end()
		buffer.place_cursor(position)
		
		# after placing the cursor at the end of line
		# return false to keep propagation, so 
		# hiting enter will move to next line
		# but not breaking it
		
		return False
		
		
		
	
		
	def text_insert(self, text):
	
		# check if sourceview is in focus
		sourceview = self.plugins["files_manager.files_manager"].current_file.source_view
		if not sourceview.is_focus():
			return False
		
		# get current viewing file' buffer
		self.buffer = sourceview.get_buffer()
		
		# get selection bound
		selection = self.buffer.get_selection_bounds()
		
		# if no selection just add close i.e. "')}]>
		if selection == ():
			return self.add_close(text, self.buffer)
		else:
			# if user selected text, then enclose
			return self.add_enclose(text, self.buffer, selection)
	
	
	
	def add_close(self, text, buffer):
		text = self.chars[text]
		text += self.close[text]
		
		position = buffer.get_iter_at_mark(buffer.get_insert())
		buffer.insert(position, text)
		
		# place cursor inbetween
		position = buffer.get_iter_at_mark(buffer.get_insert())
		position.backward_char()
		buffer.place_cursor(position)
		
		# stop propagation
		return True
	
	
	
	def add_enclose(self, text, buffer, selection):
		(start, end) = selection
		start_mark = buffer.create_mark("startclose", start, False)
		end_mark = buffer.create_mark("endclose", end, False)
		
		buffer.begin_user_action()
		
		t = self.chars[text]
		buffer.insert(start, t)
		end = buffer.get_iter_at_mark(end_mark)
		t = self.close[t]
		buffer.insert(end, t)
		
		
		
		start = buffer.get_iter_at_mark(start_mark)
		end = buffer.get_iter_at_mark(end_mark)
		end.backward_char()
		buffer.select_range(start, end)
		
		buffer.end_user_action()
		
		# stop propagation
		return True
	
	