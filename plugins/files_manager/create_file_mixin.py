
from .file import File


 
class CreateFileMixin(object):		
		
	def create_new_file(self):
		# get new sourceview from sourceview_manager
		# TODO: must handled by ui manager
		newsource = self.sourceview_manager.get_new_sourceview()
		
		newfile = File(self, f"New File {self.counter}", newsource, new_file=True)

		# add empty/current_file to files array
		# self.files.append(newfile)
		self.add_file_to_list(newfile)
		
		self.plugins["ui_manager.ui_manager"].add_filename_to_ui(newfile)
		self.switch_to_file(len(self.files) - 1)	
		
		self.counter += 1
		self.current_file.set_editted()
		
