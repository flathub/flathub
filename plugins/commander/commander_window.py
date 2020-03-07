#
#### Author: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 18th, 2020
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

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

from .window_events import WindowEvents
from .list_events import ListEvents
from .search_events import SearchEvents
from .continue_result_type import ContinueResultType
	

class CommanderWindow(WindowEvents, ListEvents, SearchEvents):


	def __init__(self, app, commander):
		self.app = app
		self.builder = app.builder
		self.commander = commander
		self.window = None
		self.listbox = None
		self.window = self.builder.get_object("commanderWindow")
		self.commanderSearchEntry = self.builder.get_object("commanderSearchEntry")
		self.listbox = self.builder.get_object("commanderList")

		self.previous_search = ""
				
		# when search, first command must be highlighted
		self.selected_first_row = None
		
		# this is to fix press down from search 
		# to move selection to the seacond row 
		# since the first row is already selected
		self.prepare_second_row = None
		
		self.scroll_in = ContinueResultType.STRICT


	
	def show_commander_window(self):
		self.remove_all_commands()
	
		self.add_commands()

		# must empty search every time showing commander
		self.previous_search = ""
		self.commanderSearchEntry.set_text("")
		
		# get the focus to search to let user type right away
		self.commanderSearchEntry.grab_focus()
				
		# unselect_all previously selected row
		self.listbox.unselect_all()
			
		self.window.show()
		
		# unhighlight first row when show commander
		self.selected_first_row = None
		
		
	
	def remove_all_commands(self):
		rows = self.listbox.get_children()
		for r in rows:
			self.listbox.remove(r)
		
		
		
	def add_commands(self):
		first = self.commander.commands_tree.first(max_result=20)
		
		self.scroll_in = ContinueResultType.NEXT
		
		# loop through commands,
		for c in first:
			self.add_command(c)
		
		self.listbox.show_all()
			
			
	def add_command(self, c):
		
		# put each in gtkbox (name, shurtcut) and bind the command 
		box = Gtk.Box.new(Gtk.Orientation.HORIZONTAL, 0)
		
		# box doesn't have command, but in python
		# we are allowed to bind any method in run time
		# we can get command from activated rows
		box.command = c
		
		lblName = Gtk.Label.new(c['name'])
		lblShortcut = Gtk.Label.new(c['shortcut'])
		box.pack_start(lblName, False, False, 0)
		box.pack_end(lblShortcut, False, False, 0)

		# adding styles
		box.get_style_context().add_class("commanderRow")
		lblName.get_style_context().add_class("commanderCommandName")
		lblShortcut.get_style_context().add_class("commanderCommanShortcut")
		
		# add to listbox
		self.listbox.insert(box, -1)
				
	

	def run_command(self, command):
		if "parameters" in command:
			p = command["parameters"]
			command["ref"](p)
		else:
			command["ref"]()
			
		# splay command
		self.commander.commands_tree.splay(command["node"])
		#self.commander.commands_tree.traverse(0)
			
		self.close()
