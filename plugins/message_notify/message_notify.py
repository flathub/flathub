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
#
#	message_notify: is a plugin responsible of displaying notification
#					messages (file saved, opened file, ...)
#	the default messageLbl is placed on top right corner
#	
#	default message_time = 7.5, which is the time of a message being displayed
#					after this time, the message will be cleared
#
#	using thread to unblock the process, thread is calling threading.Timer
#	and can cancel timed thread by calling cancel method
#

import threading
import time


class Plugin():
	
	def __init__(self, app):
		self.name = "message_notify"
		self.app = app
		self.builder = app.builder
		self.commands = []
		self.messageLbl = None
		self.message_time = 7.5 # seconds
		self.timer = None
		
	
	def activate(self):
		self.messageLbl = self.builder.get_object("messageLbl")
		self.clear_message()
			
	
	# show message (m) in messageLbl
	# set the thread timer to clear this
	# message after "message_time" seconds
	def show_message(self, m, state=0):
		self.cancel()
		
		self.messageLbl.set_text(m)
		self.update_style(state)
		self.timer = threading.Timer(self.message_time, self.clear_message)
		self.timer.start()
	
	
	
	
	def update_style(self, state):
		self.messageLbl.get_style_context().remove_class("messageImportant")
		self.messageLbl.get_style_context().remove_class("messageSuccess")
		self.messageLbl.get_style_context().remove_class("messageFail")
		
		#if state == 0:
			# do nothing just remove all css classes	
		if state == 1:
			self.messageLbl.get_style_context().add_class("messageImportant")
		elif state == 2:
			self.messageLbl.get_style_context().add_class("messageSuccess")
		elif state == 3: 
			self.messageLbl.get_style_context().add_class("messageFail")
			
	
	
	# removes any text in messageLbl
	def clear_message(self):
		self.timer = None
		self.messageLbl.set_text("")
	
	
	# cancel the timer thread and clear messageLbl
	def cancel(self):
		if self.timer:
			self.timer.cancel()
		
		self.clear_message()
		
	
