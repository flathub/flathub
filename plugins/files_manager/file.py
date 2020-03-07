#
# each openned file is set in File object and
# appended to "files" array
class File():
	def __init__(self, files_manager, filename, source_view, ui_ref=None,
					need_save=False, new_file=False, init_file=False):
		self.files_manager = files_manager
		self.filename = filename
		self.source_view = source_view
		self.ui_ref = ui_ref
		self.init_file = init_file
		self.need_save = need_save
		self.new_file = new_file
		self.editted = False
		self.parent_dir = None
		
		# if source_view is not empty,
		# then connect buffer changed signal
		# to detect if file has been editted
		if self.source_view:
			buffer = self.source_view.get_buffer()
			buffer.connect("changed", self.buffer_changed)
			
		
		
	
	def buffer_changed(self, buffer):
		self.set_editted(buffer)
		
	
	def set_editted(self, buffer=None):
		
		# if empty init file
		if self.init_file:
			self.editted_init_file(buffer)
			return 
			
	
		# if already set editted, exit
		# until reset when saved
		if self.editted:
			return
		
		# set itself editted
		self.editted = True
		self.files_manager.editted_counter += 1
		
		# print(f"editted {self.filename}")
		
		# show save all "S" menu in color
		window_ctrl = self.files_manager.plugins["window_ctrl.window_ctrl"]
		window_ctrl.grap_attention(window_ctrl.S)
		
		# change ui file appearance
		self.files_manager.plugins["ui_manager.ui_manager"].set_editted(self.ui_ref)
		

	def reset_editted(self):		
		# reset editted
		self.editted = False
		self.files_manager.editted_counter -= 1
		
		# print(f"reset editted {self.filename}")
		
		# change ui file appearance
		self.files_manager.plugins["ui_manager.ui_manager"].reset_editted(self.ui_ref)
		
		# check if need to show save all "S" menu in color
		window_ctrl = self.files_manager.plugins["window_ctrl.window_ctrl"]
		files = self.files_manager.files
		for f in files:
			if f.editted:
				window_ctrl.grap_attention(window_ctrl.S)
				return
		
		window_ctrl.remove_attention(window_ctrl.S)



	def editted_init_file(self, buffer):
		if buffer.get_char_count() == 0:
			return
					
	
		# create new file
		self.files_manager.create_new_file()
		
		# move what is been writing to new file
		text = buffer.get_text(buffer.get_start_iter(), buffer.get_end_iter(), True)
		current_buffer = self.files_manager.current_file.source_view.get_buffer()
		current_buffer.set_text(text)

		
		
