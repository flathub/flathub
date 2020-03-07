

class ListFunctionsMixin(object):

	# any text changes in search entry,
	# list is going to refilter, resort (see on_commanderSearchEntry_changed)
	# if 
	def filter(self, row, *user_data):
		# commanderWindow = user_data[0]
		searchEntry = user_data[0] 
		search_text = searchEntry.get_text().lower()
		show = False
		
		# if empty, show all
		if not search_text:
			self.selected_first_row = row
			self.listbox.select_row(self.listbox.get_row_at_index(0))
			self.prepare_second_row = self.listbox.get_row_at_index(1)
			return True
		
		box = row.get_child()
		
		# get command name
		row_text = box.get_children()[0].get_text().lower()
		
		# get command shortcut
		row_text += " " + box.get_children()[1].get_text().lower()
		show = (row_text.find(search_text) != -1)
			
		# this section is to set which row is highlighted/selected 
		# logic: first shown row, and second shown row 
		# second is used when user hit "Down" key 
		# it turnned out that we have to get the second 
		# row ref :/
		if not self.selected_first_row and show:
			self.selected_first_row = row
			self.listbox.select_row(row)
		elif not self.prepare_second_row and show:
			self.prepare_second_row = row
			
		return show
		


	# sort based on search term appeard first in command name 
	def sort(self, row1, row2, *user_data):
		search = user_data[0].get_text().lower()
		
		# if search is empty, always sor ascending
		if not search:
			return -1
		
		command1 = row1.get_child().get_children()[0].get_text().lower()
		command2 = row2.get_child().get_children()[0].get_text().lower()
		index1 = command1.find(search)
		index2 = command2.find(search)
		
		# if not found ".find" returns -1, which misses up (index1 - index2)
		# need to fix
		if index1 == index2: # either they have the search term or not
			return 0
			
		elif index1 == -1:	# if command1 doesn't have the search term
			return 1		# then return command2 before
			
		elif index2 == -1:	# otherwise
			return -1
		
		# at this point command 1 and 2 both have the search term
		
		# < 0 if row1 should be before row2 , 0 if they are equal and > 0 otherwise
		return (index1 - index2)
		
		