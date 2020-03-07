
import gi
from gi.repository import Gdk

from .continue_result_type import ContinueResultType

class SearchEvents(object):


	# The “search-changed” signal is emitted with a short delay of
	# 150 milliseconds after the last change to the entry text.
	# (see https://developer.gnome.org/gtk3/stable/GtkSearchEntry.html#GtkSearchEntry-search-changed) 
	def on_commanderSearchEntry_changed(self, widget):
		search_term = widget.get_text().lower()
		commands = self.commander.commands_tree		
		
		# reset first and second row refs
		self.selected_first_row = None
		self.prepare_second_row = None
		
		self.remove_all_commands()
		
		# when clear search 
		if not search_term:
			self.add_commands()
			self.previous_search = ""
			return
		
		
		# if user is continuing typing a word (i.e. "s", "se", "sea")
		if self.previous_search and search_term.find(self.previous_search) == 0:
			ss = commands.continue_strict_search(search_term, max_result=20)
			#print("continue_strict_search")
		else:
			ss = commands.strict_search(search_term, max_result=20)
			#print("strict_search")
		
		# when scroll, to show more results, need to know 
		# what was the previous type of iterating
		self.scroll_in = ContinueResultType.STRICT
		self.previous_search = search_term
		
		temp = []
		for c in ss:
			# print("strict", c['name'])
			self.add_command(c)
			temp.append(c)
			
		# if no enough results from strict search, then
		# do soft search 
		added_commands = len(self.listbox.get_children())
		to_add = 20 - added_commands
		soft_s = commands.soft_search(search_term, max_result=to_add)
		
		if to_add:
			self.scroll_in = ContinueResultType.SOFT
		
		for c in soft_s:
			if not c in temp:
				# print("soft", c['name'])
				self.add_command(c)
		
		
		self.listbox.unselect_all()
		self.selected_first_row = self.listbox.get_row_at_index(0)
		self.listbox.select_row(self.selected_first_row)
		self.prepare_second_row = self.listbox.get_row_at_index(1)
		self.listbox.show_all()
		
	
	
	
	
	
	
	
	
	def on_commanderSearchEntry_key_press_event(self, widget, event):
		keyval_name = Gdk.keyval_name(event.keyval)
				
		# run the first result when hit enter, or right numpad enter
		if keyval_name == "Return" or keyval_name == "KP_Enter":
			
			# get the selected row, it should be the first (filtered/sorted) row 
			first_row = self.listbox.get_selected_row()
			
			if first_row:
				self.run_command(first_row.get_child().command)
			else:
				# if no rows, then show message no commands selected
				self.app.plugins_manager.plugins["message_notify.message_notify"] \
											.show_message("No commands selected!", 3)

		
		# move to next row when press down key from searchEntry
		elif keyval_name == "Down":
			
			# if just opened the commander (i.e. no row selected)
			# then select the first row in list
			if not self.selected_first_row:
				# passing None as row makes listbox to select
				# the first row
				# shortcut of self.listbox.select_row(self.listbox.get_row_at_index(0)) 
				self.listbox.select_row(None)
			else:	
				# move to second row then focus
				# without this, user need to press "down" key twice
				# first to get listbox focus, second to move to second row
				self.listbox.select_row(self.prepare_second_row)
			
			# get the focus to listbox
			self.listbox.grab_focus()
			
