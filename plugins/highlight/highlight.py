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
#	highlight: is responsible for highlighting the selected text by user. It
#	highlights all occurrences of selected text. The highlight_signal functions
#	is connected with mark-set signal in sourceview_manager.py
#

import re # for regex

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk


class Plugin():
	
	def __init__(self, app):
		self.name = "highlight"
		self.app = app
		self.sourceview_manager = app.sourceview_manager
		self.plugins = app.plugins_manager.plugins
		self.commands = []
		self.tag_name = "search-match"
		self.spaces_pattern = re.compile("^\s+$")
		self.marks = []

		
	def activate(self):
		# connect signal mark-set event which is when user select text
		# user clicks to unselect text is also connected
		# see highlight.highlight_signal function for handling 
		# mark-set event
		self.sourceview_manager.source_view.get_buffer().connect("mark-set", self.highlight_signal)
		
		
	
	def highlight_signal(self, buffer, location, mark):
		# insert is the mark when user change
		# the cursor or select text
		if mark.get_name() == "insert":
			# gets (start, end) iterators of 
			# the selected text
			iters = buffer.get_selection_bounds()
			
			# if user only clicked/placed the cursor
			# without any selected chars, then remove
			# previously highlighted texts
			if not iters:
				# remove highlight
				self.remove_highlight(buffer)
			else:
				# when user selected some text
				# get the start and end iters
				(iter_start, iter_end) = iters
				
				# get the text is being selected, False means without tags
				# i.e. only appearing text without hidden tags set by sourceview
				# (read: https://developer.gnome.org/gtk3/stable/GtkTextBuffer.html#gtk-text-buffer-get-text)
				search = buffer.get_text(iter_start, iter_end, False)
				
				
				# if select only one letter
				if len(search) == 1 and search.isalpha():
					# remove highlight
					self.remove_highlight(buffer)
					return
				
				# if selected is only spaces
				if self.spaces_pattern.match(search):
					# remove highlight
					self.remove_highlight(buffer)
					return
		
		
				# highlight text is in seperate method
				# which help to select any text string 
				# by other plugins like find or search
				counter = self.highlight(buffer, search)
				self.plugins["message_notify.message_notify"].show_message(f"Highlighted | {counter}")
		
	
	# "search" is a string text
	# highlighting is done by adding tag(s) to
	# the text you want to highlight. The tag 
	# can have custom styling like "background color"
	# or you can copy the "search-match" style from
	# the style scheme which is set for styling the 
	# sourceview ins source_style plugin
	def highlight(self, buffer, search, search_flags=0, whole_word=True):
			
		tag = self.get_tag(buffer, self.tag_name)
		self.remove_highlight(buffer)
		
		# if search is empty, exit
		if not search:
			return
		
		# to count occurrences
		counter = 0

		# need to search for the text needed to be highlighted
		# and keep searching and taging every occurrence of the 
		# "search" text in buffer
		start_iter = buffer.get_start_iter()
		
		# gets start,end iters or None if no match
		# first search start from the beggining of the buffer
		# i.e. start_iter
		matches = start_iter.forward_search(search, search_flags, None)			
		
		# loop while still have matches (occurrences)
		while matches != None:
			# extract start, end iters from matches
			(match_start, match_end) = matches
			
			if (whole_word and self.is_whole_word(match_start, match_end)) or not whole_word:
				# set the tag to current match 
				counter += 1
				buffer.apply_tag(tag, match_start, match_end)
				s = buffer.create_mark(f"h{counter}", match_start, True)
				e = buffer.create_mark(f"eh{counter}", match_end, True)
				self.marks.append(s)
				self.marks.append(e)
							
			# do search again but start from the match_end
			# i.e. continue the search, do not search from the 
			# beggining of the file again!
			matches = match_end.forward_search(search, search_flags, None)
		
		return counter
		
		
		
		
	def is_whole_word(self, match_start, match_end):
		is_prev_a_char = True
		is_next_a_char = True
		
		prev_iter = match_start.copy()
		next_iter = match_end.copy()
		
		# move backstep
		# Returns TRUE if movement was possible; if iter was the 
		# first in the buffer (character offset 0) returns FALSE
		if not prev_iter.backward_char():
			is_prev_a_char = False
		else:
			# here the iter has moved back one step
			c = prev_iter.get_char()
			# need to check if c is not alpha nor a digit
			is_prev_a_char = (c.isalpha() or c.isdigit())
		
		
		# move forward step
		# If iter is the end iterator or one character before it,
		# iter will now point at the end iterator,
		# and gtk_text_iter_forward_char() returns FALSE
		if not next_iter:
			is_next_a_char = False
		else:
			# here the iter has moved next one step
			c = next_iter.get_char()
			# need to check if c is not alpha nor a digit
			is_next_a_char = (c.isalpha() or c.isdigit())
		
		is_word = (not is_prev_a_char and not is_next_a_char)
		 
		# both must be false to be a word
		return is_word
		
		
		
		
	
	def get_tag(self, buffer, tag_name):
		# check if tags dict has been bound to buffer
		if hasattr(buffer, "tags_dict"):
			# try to get previously created tag 
			if tag_name in buffer.tags_dict:			
				tag = buffer.tags_dict[tag_name]
				return tag
		else:
			buffer.tags_dict = {}
			
		# create new tag
		tag = buffer.create_tag(tag_name)
		
		# get the style scheme to copy "search-match" styling 
		style = buffer.get_style_scheme()
		search_tag = style.get_style("search-match")
		tag.props.background = search_tag.props.background
		tag.props.foreground = search_tag.props.foreground
		buffer.tags_dict[tag_name] = tag
	
		return tag
		
	
	# props can have:
	# - background
	# - weight
	def get_custom_tag(self, buffer, tag_name, props):
		# create new tag
		tag = self.get_tag(buffer, tag_name)
				
		if "background" in props:
			tag.props.background = props["background"]
		
		if "weight" in props:
			tag.props.weight = props["weight"]
		
		return tag
		
		
	
	def highlight_custom_tag(self, buffer, start_iter, end_iter, tag):
		self.remove_highlight(buffer, tag, start_iter, end_iter)
		buffer.apply_tag(tag, start_iter, end_iter)
		

	
	def remove_highlight(self, buffer, tag=None, start_iter=None, end_iter=None):		
		# if not cleint tag, remove all highlights
		if not tag:
			# print("a marks: ", len(self.marks))
			# delete marks too
			for m in self.marks:
				buffer.delete_mark(m)
			
			self.marks = []
			# print("b marks: ", len(self.marks))
			
			tag = self.get_tag(buffer, self.tag_name)
			
		if not start_iter:
			buffer.remove_tag(tag, buffer.get_start_iter(), buffer.get_end_iter())
		else:			
			# for performance when remove selected search
			buffer.remove_tag(tag, start_iter, end_iter)
				
						
		
			
		
		
	
	
