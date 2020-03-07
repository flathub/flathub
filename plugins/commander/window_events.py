

import gi
from gi.repository import Gdk



class WindowEvents(object):

	def on_commanderWindow_key_press_event(self, window, event):
		keyval_name = Gdk.keyval_name(event.keyval)
		ctrl = (event.state & Gdk.ModifierType.CONTROL_MASK)
		alt = (event.state & Gdk.ModifierType.MOD1_MASK)
		shift = (event.state & Gdk.ModifierType.SHIFT_MASK)
				
		# the same way as commander when open commander window,
		# this will close it with the same key
		if not alt:
			self.only_alt = True
		else:
			self.only_alt = False
			
		# also if press escape then close commander
		if keyval_name == "Escape":
			self.close()



	# if user preseed and released alt key, commander will close
	def on_commanderWindow_key_release_event(self, window, event):
		keyval_name = Gdk.keyval_name(event.keyval)
		ctrl = (event.state & Gdk.ModifierType.CONTROL_MASK)
		alt = (event.state & Gdk.ModifierType.MOD1_MASK)
		shift = (event.state & Gdk.ModifierType.SHIFT_MASK)
		
		if alt and self.only_alt and keyval_name == "Alt_L":
			self.close()
			
			
	# if user clicked outside commander window then close
	def on_commanderWindow_focus_out_event(self, window, d):
		self.close()
		

	# use hide to not lose the widgets from builder		
	def close(self):
		self.window.hide()
