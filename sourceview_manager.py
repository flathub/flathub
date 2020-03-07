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
# #
# SourceViewManager is responsible for sourceview related functions
# - get new source view
# - detect the language of the just openned file and set the langauge (i.e. C,Python,C++ ..)
# - update source map (mini map) to connect to a sourceview
#

import gi
gi.require_version('GtkSource', '4')
from gi.repository import GtkSource

class SourceViewManager():
	def __init__(self, app):
		self.app = app
		self.plugins = app.plugins_manager.plugins
		self.signal_handler = app.signal_handler
		self.source_view = app.builder.get_object("view")
		self.source_view.grab_focus()
		self.sourcemap = app.builder.get_object("sourcemap")
		self.sourcemap.set_view(self.source_view)
		self.source_view.sourcemap = self.sourcemap
		self.source_view.set_background_pattern(app.config['show_grid'])

		
	
	# opening new file needs new sourceview object
	#  here where the new sourceview object is created
	# - it copies the default sourceview properties
	# - sets the source style
	# - connects signal mark-set event which is when user select text
	# - updates the world completion to include new source buffer
	def get_new_sourceview(self):
					
		# get new sourceview object
		newsource = GtkSource.View.new()		
		newsourcemap = GtkSource.Map.new()
				
		# copy the default sourceview properties
		newsource.set_visible(self.source_view.get_visible())
		newsource.set_can_focus(self.source_view.get_can_focus())
		newsource.set_pixels_above_lines(self.source_view.get_pixels_above_lines())
		newsource.set_pixels_below_lines(self.source_view.get_pixels_below_lines())
		newsource.set_left_margin(self.source_view.get_left_margin())
		newsource.set_right_margin(self.source_view.get_right_margin())
		newsource.set_bottom_margin(self.source_view.get_bottom_margin())
		newsource.set_top_margin(self.source_view.get_top_margin())
		newsource.set_monospace(self.source_view.get_monospace())
		newsource.set_show_line_numbers(self.source_view.get_show_line_numbers())
		newsource.set_show_line_marks(self.source_view.get_show_line_marks())
		newsource.set_tab_width(self.source_view.get_tab_width())
		newsource.set_auto_indent(self.source_view.get_auto_indent())
		newsource.set_highlight_current_line(self.source_view.get_highlight_current_line())
		newsource.set_background_pattern(self.source_view.get_background_pattern())
		newsource.set_smart_home_end(self.source_view.get_smart_home_end())
	
		
		# set the source style
		self.plugins["styles.source_style"].set_source_style(newsource)
		
		# add "sourceviewclass" css class
		newsource.get_style_context().add_class("sourceviewclass")
		
		newsource.sourcemap = newsourcemap
		
		
		# connect signal mark-set event which is when user select text
		# user clicks to unselect text is also connected
		# see highlight.highlight_signal function for handling 
		# mark-set event
		newsource.get_buffer().connect("mark-set", self.plugins["highlight.highlight"].highlight_signal)
		
		# when creating new buffer,
		# share this buffer to whom need it
		self.signal_handler.emit("sourceview-created", newsource)
		
		# show the gtk widget
		newsource.show()

		
		# TODO: move to files_manager, sometimes we don't need to 
		# update completion based on file type and size!
		# update the world completion to include new source buffer
		self.plugins["simple_completion.simple_completion"].update_completion(newsource)
		
		return newsource
		
	
	
	
	
	# detect the language of the just openned file 
	# and set the langauge (i.e. C,Python,C++ ..)
	def set_language(self, filename, buffer):
		lm = GtkSource.LanguageManager.get_default()
		
		# guess the language of the filename
		lan = lm.guess_language(filename)
		if lan:
			# set the highlight of buffer
			buffer.set_highlight_syntax(True)
			buffer.set_language(lan)
		else:
			print('No language found for file "cen"')
			buffer.set_highlight_syntax(False)
			
			

