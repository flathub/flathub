#
#### Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
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
#	Copy, cut, or duplicate a line without selecting all line. Just
#	place the cursor in the line and press Ctrl+c for copy, Ctrl+x to
#	cut, or Ctrl+d to duplicate. Also after copying and cutting, pressing
#	Ctrl+v will paste into next line.
#
#
#


from . import commands
from .copy_line import CopyLine
from .cut_line import CutLine
from .duplicate_line import DuplicateLine
from .special_paste import SpecialPaste

class Plugin(SpecialPaste, DuplicateLine, CutLine, CopyLine):
	
	def __init__(self, app):
		self.name = "fast_copy_cut_duplicate"
		self.app = app
		self.plugins = app.plugins_manager.plugins
		self.signal_handler = app.signal_handler 
		self.commands = []
		self.copied_line = ""
		self.dont_propagate_paste = False
		
	
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)

	
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if ctrl and keyval_name == "c":
			self.copy_line()
		elif ctrl and keyval_name == "x":
			self.cut_line()
		elif ctrl and keyval_name == "d":
			self.duplicate_line()
		elif ctrl and keyval_name == "v":
			self.dont_propagate_paste = False
			self.special_paste()
			return self.dont_propagate_paste
		


	def discard_white_spaces(self, iter):
		whitespaces = ""
		while not iter.ends_line():
			# get char where the current iter pointing to
			c = iter.get_char()
			
			# check if c is not white space
			if c != " " and c != "\t":
				return (iter, whitespaces)
			
			iter.forward_char()
			whitespaces += c

		return (iter, whitespaces)
		
		