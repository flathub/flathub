
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk



class SpecialPaste(object):

	def special_paste(self):
		clipboard = Gtk.Clipboard.get_default(Gdk.Display.get_default())
		
		# get current viewing file' buffer
		self.buffer = self.plugins["files_manager.files_manager"].current_file.source_view.get_buffer()
		
		# get selection bound
		selection = self.buffer.get_selection_bounds()
		
		# if user selected text, then paste normally
		# so propagate 
		if selection != ():
			self.dont_propagate_paste = False
			return
		
		
		# TODO: callback sometimes delays
		# makes problem with return propagate
		# if returned True, means do not propagate
		# i.e. no default paste 
		clipboard.request_text(self.copied_text)

		
		
	def copied_text(self, clipboard, text):
		if text == self.copied_line:				
			self.dont_propagate_paste = True
			self.do_special_paste(text)
		else:
			self.dont_propagate_paste = False
			self.copied_line = ""
			
			
	def do_special_paste(self, text):
		# get current viewing file' buffer
		self.buffer = self.plugins["files_manager.files_manager"].current_file.source_view.get_buffer()
		
		# get selection bound
		selection = self.buffer.get_selection_bounds()
		currentPosMark = self.buffer.get_insert()
		start = self.buffer.get_iter_at_mark(currentPosMark)
		start.set_line_offset(0)
		end = start.copy()
		if not end.ends_line():
			end.forward_to_line_end()
		
		# get current whitespaces
		(start, whitespaces) = self.discard_white_spaces(start)
		
		# insert new line + whitespaces + text
		paste_text = "\n" + whitespaces + text
		
		self.buffer.insert(end, paste_text)
		
		# get end iters position to replace the cursor
		current_cursor = self.buffer.get_insert()
		c_iter = self.buffer.get_iter_at_mark(current_cursor)
		
		# if at the end of the line
		# read gtk_text_iter_forward_line ()
		if c_iter.ends_line():
			# move to next line
			c_iter.forward_line()
			self.buffer.place_cursor(c_iter)
		else:
			# count offset from start of line to current cursor
			offset = c_iter.get_line_offset()
			
			# move to next line
			c_iter.forward_line()
			
			# move same offsets
			c_iter.forward_chars(offset)
			
			self.buffer.place_cursor(c_iter)
		
		
		
