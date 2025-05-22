from gi.repository import Gio
from .proxy import ProxyMixin
from .request_name import RequestNameMixin
from .bus_names import OwnMixin, WatchMixin
from .subscription import SubscriptionMixin
from .registration import RegistrationMixin
from .publication import PublicationMixin

def pydbus_property(self):
	try:
		return self._pydbus
	except AttributeError:
		self._pydbus = Bus(self)
		return self._pydbus

Gio.DBusConnection.pydbus = property(pydbus_property)

def bus_get(type):
	return Gio.bus_get_sync(type, None).pydbus

def connect(address):
	c = Gio.DBusConnection.new_for_address_sync(address, Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT | Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION, None, None)
	c.pydbus.autoclose = True
	return c.pydbus

class Bus(ProxyMixin, RequestNameMixin, OwnMixin, WatchMixin, SubscriptionMixin, RegistrationMixin, PublicationMixin):
	Type = Gio.BusType

	def __init__(self, gio_con):
		self.con = gio_con
		self.autoclose = False

	def __enter__(self):
		return self

	def __exit__(self, exc_type, exc_value, traceback):
		if self.autoclose:
			self.con.close_sync(None)

	@property
	def dbus(self):
		try:
			return self._dbus
		except AttributeError:
			self._dbus = self.get(".DBus")[""]
			return self._dbus

	@property
	def polkit_authority(self):
		try:
			return self._polkit_authority
		except AttributeError:
			self._polkit_authority = self.get(".PolicyKit1", "Authority")[""]
			return self._polkit_authority

def SystemBus():
	return bus_get(Bus.Type.SYSTEM)

def SessionBus():
	return bus_get(Bus.Type.SESSION)
