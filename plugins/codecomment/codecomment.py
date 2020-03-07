#
#
#  Code comment plugin
#  This file is part of gedit
#
#  Copyright (C) 2005-2006 Igalia
#  Copyright (C) 2006 Matthew Dugan
#  Copyrignt (C) 2007 Steve Frécinaux
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA 02110-1301, USA.
#
#
#
# Editted By: Hamad Al Marri <hamad.s.almarri@gmail.com>
# Date: Feb 19th, 2020
#
#

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

from .codecomment_tags import CodeCommentTags
from . import commands

class Plugin(CodeCommentTags):
	

	def __init__(self, app):
		self.name = "codecomment.codecomment2"
		self.app = app
		self.plugins = app.plugins_manager.plugins
		self.signal_handler = app.signal_handler
		self.commands = []
		
	
	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)
		
		

	def key_bindings(self, event, keyval_name, ctrl, alt, shift):		
		if ctrl and keyval_name == "slash":
			self.do_comment()
			return True
			
			


	def do_comment(self):
		sourceview = self.plugins["files_manager.files_manager"].current_file.source_view
		buffer = sourceview.get_buffer()
		
		lang = buffer.get_language()
		if lang is None:
			return
		
		# get tags from .lan file related to current files
		# if .c file opened (tags are /* */ or //)
		# if .py file opened (tags #) and so on
		(start_tag, end_tag) = self.get_comment_tags(lang)
		if not start_tag and not end_tag:
			return
			
		
		# get user selection
		sel = buffer.get_selection_bounds()
		currentPosMark = buffer.get_insert()
		oldPos = 0
		
		# if user selected chars or multilines
		if sel != ():
			deselect = False
			(start, end) = sel
			if not start.starts_line():
				start.set_line_offset(0)
			if not end.ends_line():
				end.forward_to_line_end()
		
		# if user not selecting any chars (i.e. just placed cursor in a line)
		else:
			deselect = True
			start = buffer.get_iter_at_mark(currentPosMark)
			oldPos = buffer.get_iter_at_mark(currentPosMark).get_offset()
			start.set_line_offset(0)
			end = start.copy()
			if not end.ends_line():
				end.forward_to_line_end()


			
				
		# if empty line (i.e. start == end)
		if start.get_offset() == end.get_offset():
			buffer.begin_user_action()
			buffer.insert(start, start_tag)
			buffer.insert(start, " ")
			buffer.end_user_action()
			return

		
		new_code = self.add_comment_characters(buffer, start_tag, end_tag, start, end, deselect, oldPos)


	
	
	def discard_white_spaces(self, iter):
		count = 0
		while not iter.ends_line():
			# get char where the current iter pointing to
			c = iter.get_char()
			
			# check if c is not white space
			if c != " " and c != "\t":
				return (iter, count)
			
			iter.forward_char()
			count += 1
		
		return (iter, 0)
		
	
	def is_commented(self, comment_pos_iter, start_tag):
		head_iter = comment_pos_iter.copy()
		self.forward_tag(head_iter, start_tag)
		s = comment_pos_iter.get_slice(head_iter)
		if s == start_tag:
			return True
		
		return False



	def add_comment_characters(self, document, start_tag, end_tag, start, end, deselect, oldPos):
		smark = document.create_mark("start", start, False)
		imark = document.create_mark("iter", start, False)
		emark = document.create_mark("end", end, False)
		number_lines = end.get_line() - start.get_line() + 1
		comment_pos_iter = None
		count = 0
		
		document.begin_user_action()

		for i in range(0, number_lines):
			iter = document.get_iter_at_mark(imark)
			if not iter.ends_line():
				
				if not comment_pos_iter:
					(comment_pos_iter, count) = self.discard_white_spaces(iter)
					
					# check if already commented
					if self.is_commented(comment_pos_iter, start_tag):
						new_code = self.remove_comment_characters(document, start_tag, end_tag, start, end)
						return
						
				else:
					comment_pos_iter = iter
					# move iter to match first alignment
					for i in range(count):
						iter.forward_char()
					
				document.insert(comment_pos_iter, start_tag)
				
				# also insert a space
				document.insert(comment_pos_iter, " ")
				
				# if block tag (/*    */) style
				if end_tag:
					# if not the last selected line
					if i != number_lines -1:
						# place the end block tag (*/) to end of line
						iter = document.get_iter_at_mark(imark)
						iter.forward_to_line_end()
						document.insert(iter, end_tag)
					else:
						# place the end block tag to end of selection
						iter = document.get_iter_at_mark(emark)
						document.insert(iter, end_tag)
					
						
			iter = document.get_iter_at_mark(imark)
			iter.forward_line()
			document.delete_mark(imark)
			imark = document.create_mark("iter", iter, True)

		document.end_user_action()

		document.delete_mark(imark)
		new_start = document.get_iter_at_mark(smark)
		new_end = document.get_iter_at_mark(emark)
		# if not new_start.ends_line():
		#	self.backward_tag(new_start, start_tag)
		document.select_range(new_start, new_end)
		document.delete_mark(smark)
		document.delete_mark(emark)
		
		# place the cursor to its old position
		if deselect:
			oldPosIter = document.get_iter_at_offset(oldPos + 2)
			document.place_cursor(oldPosIter)
		
	
	
	
	
	def forward_tag(self, iter, tag):
		iter.forward_chars(len(tag))

	def backward_tag(self, iter, tag):
		iter.backward_chars(len(tag))
		
		

	def get_tag_position_in_line(self, tag, head_iter, iter):
		while not iter.ends_line():
			# get text starting from iter to head_iter
			s = iter.get_slice(head_iter)
			if s == tag:
				return True
			else:
				head_iter.forward_char()
				iter.forward_char()
		return False
		
		
	def remove_comment_characters(self, document, start_tag, end_tag, start, end):
		smark = document.create_mark("start", start, False)
		emark = document.create_mark("end", end, False)
		number_lines = end.get_line() - start.get_line() + 1
		iter = start.copy()
		head_iter = iter.copy()
		self.forward_tag(head_iter, start_tag)

		document.begin_user_action()

		for i in range(0, number_lines):
			# print(f"line {i}")
			if self.get_tag_position_in_line(start_tag, head_iter, iter):
				dmark = document.create_mark("delete", iter, False)
				document.delete(iter, head_iter)
				
				# delete the extra space added
				space_iter = head_iter.copy()
				space_iter.forward_char()
				s = head_iter.get_slice(space_iter)
				if s == " ":
					# remove 
					document.delete(head_iter, space_iter)
				
				if end_tag:
					iter = document.get_iter_at_mark(dmark)
					head_iter = iter.copy()
					self.forward_tag(head_iter, end_tag)
					if self.get_tag_position_in_line(end_tag, head_iter, iter):
						document.delete(iter, head_iter)
				document.delete_mark(dmark)
				
			iter = document.get_iter_at_mark(smark)
			iter.forward_line()
			document.delete_mark(smark)
			head_iter = iter.copy()
			self.forward_tag(head_iter, start_tag)
			smark = document.create_mark("iter", iter, True)

		document.end_user_action()

		document.delete_mark(smark)
		document.delete_mark(emark)
