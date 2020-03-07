#
#### Author: Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 25th, 2020
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


import os

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk


class FindReplaceWindow(object):
	
	def set_handlers(self):
		self.signals = {
			"on_window_delete_event": self.on_window_delete_event,
			"on_find_replace_window_key_press_event": self.on_find_replace_window_key_press_event,
			"on_find_prev_btn_clicked": self.on_find_prev_btn_clicked,
			"on_find_btn_clicked": self.on_find_btn_clicked,
			"on_replace_btn_clicked": self.on_replace_btn_clicked,
			"on_replace_all_btn_clicked": self.on_replace_all_btn_clicked,
			"on_match_case_btn_toggled": self.on_match_case_btn_toggled,
			"on_whole_world_btn_toggled": self.on_whole_world_btn_toggled,
			"on_close_find_btn_clicked": self.on_close_find_btn_clicked,
			"on_replace_expander_button_press_event": self.on_replace_expander_button_press_event,
			}
		
	
	def load_ui(self):
		dir_path = os.path.dirname(os.path.realpath(__file__))
		self.builder = Gtk.Builder()
		self.builder.add_from_file(f"{dir_path}/ui.glade")
		
		self.builder.connect_signals(self.signals)
		
		self.find_text_view = self.builder.get_object("find_text_view")
		self.find_text_view.get_buffer().connect("changed", self.on_find_text_view_changed)
		
		self.replace_text_view = self.builder.get_object("replace_text_view")
		self.find_status_lbl = self.builder.get_object("find_status_lbl")
				
		self.window = self.builder.get_object("find_replace_window")
		self.window.set_transient_for(self.app.window)
		
		
	
	def hide(self):
		self.window.hide()
		# remove color from "F and R" menu
		window_ctrl = self.plugins["window_ctrl.window_ctrl"]
		window_ctrl.remove_attention(window_ctrl.F)
		window_ctrl.remove_attention(window_ctrl.R)
		
		self.new_search = True
		self.clear_highlights()
		
		
		
	def show_window(self, show_replace=False):
		self.show_replace = show_replace
		if not self.window:
			self.load_ui()	
		
		replace_expander = self.builder.get_object("replace_expander")
		replace_expander.set_expanded(self.show_replace)
		
		self.fill_findtext()

		if not self.show_replace and self.window.get_visible():
			self.hide()
		else:
			self.window.show_all()
			# show color for "F and R" menu
			window_ctrl = self.plugins["window_ctrl.window_ctrl"]
			window_ctrl.grap_attention(window_ctrl.F)
			if self.show_replace:
				window_ctrl.grap_attention(window_ctrl.R)



	def fill_findtext(self):
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
			
			self.find_text_view.get_buffer().set_text(text)
		
		# if not text selection, check the top search entry
		else:
			searchEntry = self.app.builder.get_object("searchEntry")
			text = searchEntry.get_text()
			if text:
				self.find_text_view.get_buffer().set_text(text)
		
		
		
	def on_find_replace_window_key_press_event(self, window, event):
		keyval_name = Gdk.keyval_name(event.keyval)
		ctrl = (event.state & Gdk.ModifierType.CONTROL_MASK)
		alt = (event.state & Gdk.ModifierType.MOD1_MASK)
		shift = (event.state & Gdk.ModifierType.SHIFT_MASK)
		
		if keyval_name == "Escape":
			self.hide()
		 
	
	def on_window_delete_event(self, w, e):
		self.hide()
		return True
	
	
	def on_find_prev_btn_clicked(self, w):
		self.do_find(previous=True)
		
		
	def on_find_btn_clicked(self, w):
		self.do_find()
	
	
	
	def on_replace_btn_clicked(self, w):
		self.do_replace()
	
	def on_replace_all_btn_clicked(self, w):
		self.do_replace_all()
	
	def on_match_case_btn_toggled(self, w):
		self.match_case = w.get_active()
		self.new_search = True
		
	
	def on_whole_world_btn_toggled(self, w):
		self.whole_word = w.get_active()
		self.new_search = True
		
	
	def on_close_find_btn_clicked(self, w):
		self.hide()


	def on_find_text_view_changed(self, buffer):
		self.new_search = True


	def on_replace_expander_button_press_event(self, w, e):
		window_ctrl = self.plugins["window_ctrl.window_ctrl"]
		
		if w.get_expanded():
			# remove color from "F and R" menu
			window_ctrl.remove_attention(window_ctrl.R)
		else:
			window_ctrl.grap_attention(window_ctrl.R)