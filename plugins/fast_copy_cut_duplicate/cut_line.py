

class CutLine(object):

	def cut_line(self):
		line_iters = self.copy_line()
		
		if not line_iters:
			return
			
		(start, end) = line_iters
		
		# the line is already in clipboard
		# delete line from buffer
		
		# get current viewing file' buffer
		self.buffer = self.plugins["files_manager.files_manager"].current_file.source_view.get_buffer()
		
		# after discard_white_spaces, need to position start at line begining
		start.set_line_offset(0)
		
		# delete the \n as well
		end.forward_char()
		
		# delete line 
		self.buffer.delete(start, end)
		
		
	
	
	