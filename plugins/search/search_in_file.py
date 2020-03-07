#
#### Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 16th, 2020
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
#	Quick search on current opened file. It is located at
#	the top of the editor. It auto scroll to first found
#	match and moving to the next/previous one by UP/DOWN keywords or Enter/Shift+Enter.
#	It is case sensitive. For case insensitive, see find_and_replace plugin.
#

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk
from . import commands

class Plugin(): 
	
	def __init__(self, app):
		self.name = "search_in_file"
		self.app = app
		self.signal_handler = app.signal_handler
		self.handlers = app.signal_handler.handlers
		self.builder = app.builder
		self.plugins = app.plugins_manager.plugins
		self.sourceview = None
		self.buffer = None
		self.commands = []
		self.searchEntry = None
		self.search = None
		self.search_flags = 0
		self.whole_word = False
		self.is_highlight_done = False
		self.count = 0
		self.match_number = 0
		self.deleted_marks = 0
		self.old_start_iter = None
		self.old_end_iter = None
		self.current_selection = None
		self.signal_handler.connect("file-switched", self.refresh_source)
		self.searchEntry = self.builder.get_object("searchEntry")
		
		
		# for highlight current match
		self.props = {
			"weight": 1700,
			"background": "#dddd77",
		}
		self.tag = None
		self.tag_name = "selected_search"
		
				
	
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		self.set_handlers()
				
		commands.set_commands(self)
	

	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if ctrl and keyval_name == "f":
			self.get_focus()
			
	
	# setting handlers, see SignalHandler
	def set_handlers(self):
		self.handlers.on_search_field_changed = self.on_search_field_changed
		self.handlers.on_search_key_press_event = self.on_search_key_press_event
		self.handlers.on_search_focus_out_event = self.on_search_focus_out_event
		
	
	def refresh_source(self, new_source):
		self.quit_search()
		h = self.plugins["highlight.highlight"]
		self.sourceview = new_source
		self.buffer = self.sourceview.get_buffer()
		self.tag = h.get_custom_tag(self.buffer, self.tag_name, self.props)
		
	
	
	def on_search_key_press_event(self, widget, event):
		keyval_name = Gdk.keyval_name(event.keyval)
		shift = (event.state & Gdk.ModifierType.SHIFT_MASK)
		
		if keyval_name == "Escape":
			
			# before self.clear_search(before loosing search marks)
			# try to place the cursor on current selected text in sourceview
			self.place_cursor_to_selection()
			
			self.clear_search(widget)
			
			# set focus back to sourceview
			self.plugins["files_manager.files_manager"].current_file.source_view.grab_focus()
			
		elif (shift and keyval_name == "Return") or keyval_name == "Up":
			# self.refresh_sources()
			
			self.search_flags = 0
			self.whole_word = False
			if not self.is_highlight_done:
				self.do_highlight(self.searchEntry.get_text(), self.buffer)
			self.scroll_prev()	
			
		elif keyval_name == "Return" or keyval_name == "KP_Enter" or keyval_name == "Down":
			# self.refresh_sources()
			
			self.search_flags = 0
			self.whole_word = False
			if not self.is_highlight_done:
				self.do_highlight(self.searchEntry.get_text(), self.buffer)
			else:
				self.scroll_next()
				
		
	
	def on_search_focus_out_event(self, widget, data):
		self.quit_search()
		
		
	def place_cursor_to_selection(self):
		if self.old_start_iter:
			self.buffer.place_cursor(self.old_start_iter)
		
			
	def quit_search(self):
		self.is_highlight_done = False
		if self.buffer:
			self.plugins["highlight.highlight"].remove_highlight(self.buffer, self.tag)
		self.set_selected_iters(None, None)
		if self.buffer:
			self.plugins["highlight.highlight"].remove_highlight(self.buffer)
		self.update_style(-1)
	
	
	def get_focus(self):
				
		# gets (start, end) iterators of 
		# the selected text
		iters = self.buffer.get_selection_bounds()
		if iters:
			# when user selected some text
			# get the start and end iters
			(iter_start, iter_end) = iters
			
			# get the text is being selected, False means without tags
			# i.e. only appearing text without hidden tags set by sourceview
			# (read: https://developer.gnome.org/gtk3/stable/GtkTextBuffer.html#gtk-text-buffer-get-text)
			text = self.buffer.get_text(iter_start, iter_end, False)
			
			self.searchEntry.set_text(text)

		# set cursor to searchEntry
		self.searchEntry.grab_focus()
		self.update_style(0)
				
	
	
	
	def update_style(self, state):
		self.searchEntry.get_style_context().remove_class("searching")
		self.searchEntry.get_style_context().remove_class("searchSuccess")
		self.searchEntry.get_style_context().remove_class("searchFail")
		
		if state == 0:
			# searching in blue
			self.searchEntry.get_style_context().add_class("searching")
		elif state == 1:
			# search success in green
			self.searchEntry.get_style_context().add_class("searchSuccess")
		elif state == 2:
			# no results in red 
			self.searchEntry.get_style_context().add_class("searchFail")
			
			
	
	# (see https://developer.gnome.org/gtk3/stable/GtkSearchEntry.html)
	# (https://developer.gnome.org/gtk3/stable/GtkEntry.html)
	def on_search_field_changed(self, widget):
		# self.refresh_sources()
		
		self.search_flags = 0
		self.whole_word = False
		self.do_highlight(widget.get_text(), self.buffer)
		
	
	# (see https://developer.gnome.org/gtk3/stable/GtkSearchEntry.html)
	# (https://developer.gnome.org/gtk3/stable/GtkEntry.html)
	def do_highlight(self, search, buffer):
		
		if buffer:
			self.buffer = buffer
		
		# remove selected tag (bold text)
		self.plugins["highlight.highlight"].remove_highlight(buffer, self.tag)
		self.set_selected_iters(None, None)
		
		# print(f"search.whole_word: {self.whole_word}")
		self.search = search
		self.count = self.plugins["highlight.highlight"].highlight( buffer, \
									self.search, self.search_flags, self.whole_word)
		
		# if no results while search is not empty
		if self.count == 0 and self.search:
			self.plugins["message_notify.message_notify"].show_message("Search Results | 0")
			self.update_style(2)
			return
			
		if not self.search:
			self.update_style(0)
		else:
			self.update_style(1)
			
		self.is_highlight_done = True
		
		# scroll to first occurrence of search if not empty
		if self.search:
			self.match_number = -1
			self.deleted_marks = 0
			self.scroll_next()
		


	def scroll_next(self):		
		highlight = self.plugins["highlight.highlight"]
		marks = highlight.marks
		
		if not marks:
			return
		
		self.match_number += 1
		
		if self.match_number * 2 == len(marks):
			self.match_number = 0
		
		self.scroll(marks)

					
	
	def scroll_prev(self):
		highlight = self.plugins["highlight.highlight"]
		marks = highlight.marks
		
		if not marks:
			return
			
		if self.match_number == 0:
			self.match_number = (len(marks) // 2)
			
		self.match_number -= 1
		self.scroll(marks)
		
		
	def scroll(self, marks):
		next_mark_pos = self.match_number * 2
		match_start = self.buffer.get_iter_at_mark(marks[next_mark_pos])
		match_end = self.buffer.get_iter_at_mark(marks[next_mark_pos + 1])
		
		self.sourceview.scroll_to_mark(marks[self.match_number * 2], 0.20, False, 1.0, 0.5)
		self.plugins["message_notify.message_notify"].show_message( \
					"Search Results | " + str(self.match_number + self.deleted_marks + 1) + "/" + str(self.count))
		self.highlight_scrolled(match_start, match_end)
			
	
	
	def highlight_scrolled(self, start_iter, end_iter):
		h = self.plugins["highlight.highlight"]
		self.tag = h.get_custom_tag(self.buffer, self.tag_name, self.props)
		
		# remove old highlight
		if self.old_start_iter:
			h.remove_highlight(self.buffer, self.tag, self.old_start_iter, self.old_end_iter)
		
		h.highlight_custom_tag(self.buffer, start_iter, end_iter, self.tag)
		self.set_selected_iters(start_iter, end_iter)
		
		

	def clear_search(self, widget):
		self.search = ""
		widget.set_text(self.search)
		self.plugins["highlight.highlight"].remove_highlight(self.buffer, self.tag)
		
	
	def set_selected_iters(self, s_iter, e_iter):
		self.old_start_iter = s_iter
		self.old_end_iter = e_iter
		if s_iter:
			self.current_selection = (s_iter, e_iter)
		else:
			self.current_selection = None



	def delete_current_marks(self):
		highlight = self.plugins["highlight.highlight"]
		marks = highlight.marks
		
		s_mark = marks[self.match_number * 2]
		e_mark = marks[(self.match_number * 2) + 1]
		
		self.buffer.delete_mark(s_mark)
		self.buffer.delete_mark(e_mark)
		
		del marks[(self.match_number * 2) + 1]
		del marks[self.match_number * 2]
		
		self.match_number -= 1
		self.deleted_marks += 1
		
