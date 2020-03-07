#
#  Editted By: Hamad Al Marri <hamad.s.almarri@gmail.com>
#  Date: Feb 19th, 2020
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


 
class CodeCommentTags(object):
	
	def get_comment_tags(self, lang):
		(s, e) = self.get_line_comment_tags(lang)
		if (s, e) == (None, None):
			(s, e) = self.get_block_comment_tags(lang)
		return (s, e)


	def get_block_comment_tags(self, lang):
		start_tag = lang.get_metadata('block-comment-start')
		end_tag = lang.get_metadata('block-comment-end')
		if start_tag and end_tag:
			return (start_tag, end_tag)
		return (None, None)
		
		
	def get_line_comment_tags(self, lang):
		start_tag = lang.get_metadata('line-comment-start')
		if start_tag:
			return (start_tag, None)
		return (None, None)