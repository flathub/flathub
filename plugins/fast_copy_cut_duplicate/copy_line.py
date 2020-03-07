
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk


class CopyLine(object):
	
	def copy_line(self):
		
		# get current viewing file's buffer
		self.buffer = self.plugins["files_manager.files_manager"].current_file.source_view.get_buffer()
		
		# get selection bound
		selection = self.buffer.get_selection_bounds()
		
		# if user selected text, then exit
		# no need for fast copy a line
		if selection != ():
			self.copied_line = ""
			return None
			
		
		# if selection is empty
		# get current insert position mark (insert = cursor)
		currentPosMark = self.buffer.get_insert()
		start = self.buffer.get_iter_at_mark(currentPosMark)
		start.set_line_offset(0)
		end = start.copy()
		if not end.ends_line():
			end.forward_to_line_end()
		
		# if empty line (i.e. start == end), the exit
		if start.get_offset() == end.get_offset():
			self.copied_line = ""
			return (start, end)
		
		# copy text only without white spaces/indentations
		(start, whitespaces) = self.discard_white_spaces(start)
		
		# get line text
		line = self.buffer.get_text(start, end, False)
		
		# copy line in clipboard
		self.copy_to_clipboard(line)
		
		return (start, end)
		
		
		
	def copy_to_clipboard(self, line):
		clipboard = Gtk.Clipboard.get_default(Gdk.Display.get_default())
		
		# + \n is useful when paste line
		# the cursor moves to next line
		# good for copy one line and paste
		# multiple times
		self.copied_line = line
			
		clipboard.set_text(line, -1)
		
		
		
		
		