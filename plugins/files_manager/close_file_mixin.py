# 
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from .file import File

class CloseFileMixin(object):
	
	
	def close_all(self):
		# > 1 to not delete empty init file
		num_files = len(self.files)
		counter = 1  # < don't close empty init
		while counter < num_files:
			self.switch_to_file(num_files - counter)
			self.close_current_file()
			counter += 1
	


	def close_file(self, filename):
		# check if the current file is being closed
		if self.current_file.filename == filename:
			self.close_current_file()
			
		else:
			# get  file index
			to_close_index = self.get_file_index(filename)
			
			# destroy file
			if not self.destroy_file(to_close_index):
				return
			
			# switch to last file
			self.switch_to_file(len(self.files) - 1)

				
	
	
	def close_current_file(self):	
		# if length > 2, then close current and switch to previouse file 
		# in "files" array
		if len(self.files) > 2:
			# get current file index
			to_close_index = self.get_file_index(self.current_file.filename)
					
			# destroy file
			if not self.destroy_file(to_close_index):
				return
			
			# switch to last file
			self.switch_to_file(len(self.files) - 1)

		
		# if empty file only there, do nothing
		elif len(self.files) == 1 and self.files[0].init_file:
			return
			
			
		# if 2 files (a signle file, and empty in array), close and make empty file to stay 
		# in the view
		else:			
			# destroy opened file 
			if not self.destroy_file(1):
				return
			
			# make sure init file is empty
			self.files[0].source_view.get_buffer().set_text("")
			
			# remove current sourceview and put the new empty sourceview
			self.plugins["ui_manager.ui_manager"].replace_sourceview_widget(self.files[0].source_view)
			
			# current file is now empty
			self.current_file = self.files[0]
			
						
			# since it is an empty file, set the headerbar to "Gamma"
			self.plugins["ui_manager.ui_manager"].set_header("Gamma")
			
			# cancel and clear message 
			# why? sometimes user save a file and close it right after,
			# so no need to keep showing that file is saved
			self.plugins["message_notify.message_notify"].cancel()
		
			
		

	def destroy_file(self, file_index):
		# print(file_index)
		file_object = self.files[file_index]
		close = True
		
		if file_object.editted:
			# switch to file to let the user 
			# know which file is it
			self.switch_to_file(self.get_file_index(file_object.filename))
		
			response = self.prompt_save(file_object)
			
			# if user clicked save
			if response == 0:
				# save does the reset_editted
				self.plugins["files_manager.savefile"].save_file(file_object)
			
			# if user clicked no
			elif response == 2:
				# reduce number of editted files
				file_object.reset_editted()
			
			# if user clicked "back to edit (cancel)"
			# then cancel closing
			else:
				close = False

		if close:		
			# remove from "files" array
			#del self.files[file_index]
			self.remove_file_from_list(file_object, file_index)
		
			# destroy the ui_ref btn attached to file TODO: move to ui manager
			file_object.ui_ref.destroy()

		return close
	
	
	
	


	def prompt_save(self, file_object=None):
		filename = file_object.filename
		
		
		dialog = Gtk.MessageDialog(
			self.app.window,
			Gtk.DialogFlags.DESTROY_WITH_PARENT,
			Gtk.MessageType.WARNING, # or WARNING
			Gtk.ButtonsType.NONE,
			f'Save "{filename}" ?'
		)
		
		dialog.format_secondary_text("")
		
		dialog.add_button("Save", 0)
		dialog.add_button("Don't Close", 1)
		
		btn = Gtk.Button.new()
		btn.set_label("No")
		btn.get_style_context().add_class("destructive-action")
		btn.show()
		
		dialog.add_action_widget(btn, 2)
		dialog.set_default_response(0)
	
		# show the dialog		
		response = dialog.run()
		# close and destroy dialog object
		dialog.destroy()


		return response		
		
