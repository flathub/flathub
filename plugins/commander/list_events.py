
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

from .continue_result_type import ContinueResultType
	
class ListEvents(object):

	# activated means either row clicked or selected by keyboard and hit enter 
	def on_commanderList_row_activated(self, widget, row):
		# run the command 
		self.run_command(row.get_child().command)
		
	
	def on_commanderList_key_press_event(self, widget, event):
		keyval_name = Gdk.keyval_name(event.keyval)
		
		# go up back to searchEntry
		if keyval_name == "Up":
			first_row = self.listbox.get_row_at_y(1)
			if not first_row or first_row.is_selected():
				self.commanderSearchEntry.grab_focus_without_selecting()
				
		# if start typing again, back to searchEntry
		# and insert that key to search 
		elif keyval_name != "Return" and keyval_name != "Up" and keyval_name != "Down":
			self.commanderSearchEntry.grab_focus_without_selecting()
			
			# pass the key press to search entry
			self.commanderSearchEntry.do_key_press_event(self.commanderSearchEntry, event)



	def on_commander_list_edge_reached(self, scrolled_window, pos):
		if pos == Gtk.PositionType.BOTTOM:
			commands = self.commander.commands_tree
			search_term = self.commanderSearchEntry.get_text()
			more = None
			if self.scroll_in == ContinueResultType.NEXT:
				more = commands.next(max_result=20)
			elif self.scroll_in == ContinueResultType.STRICT:
				more = commands.continue_strict_search(search_term, max_result=20)
			elif self.scroll_in == ContinueResultType.SOFT:
				more = commands.continue_soft_search(search_term, max_result=20)
				
			for c in more:
				self.add_command(c)
				
			self.listbox.show_all()

				
