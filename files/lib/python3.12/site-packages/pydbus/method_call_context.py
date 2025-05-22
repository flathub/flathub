from gi.repository import GLib
from collections import namedtuple

AuthorizationResult = namedtuple("AuthorizationResult", "is_authorized is_challenge details")

class MethodCallContext(object):
	def __init__(self, gdbus_method_invocation):
		self._mi = gdbus_method_invocation

	@property
	def bus(self):
		return self._mi.get_connection().pydbus

	@property
	def sender(self):
		return self._mi.get_sender()

	@property
	def object_path(self):
		return self._mi.get_object_path()

	@property
	def interface_name(self):
		return self._mi.get_interface_name()

	@property
	def method_name(self):
		return self._mi.get_method_name()

	def check_authorization(self, action_id, details, interactive=False):
		return AuthorizationResult(*self.bus.polkit_authority.CheckAuthorization(('system-bus-name', {'name': GLib.Variant("s", self.sender)}), action_id, details, 1 if interactive else 0, ''))

	def is_authorized(self, action_id, details, interactive=False):
		return self.check_authorization(action_id, details, interactive).is_authorized
