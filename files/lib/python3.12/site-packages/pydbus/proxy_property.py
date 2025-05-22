from gi.repository import GLib

class ProxyProperty(object):
	def __init__(self, iface_name, property):
		self._iface_name = iface_name
		self.__name__ = property.attrib["name"]
		self.__qualname__ = self._iface_name + "." + self.__name__

		self._type = property.attrib["type"]
		access = property.attrib["access"]
		self._readable = access.startswith("read")
		self._writeable = access.endswith("write")
		self.__doc__ = "(" + self._type + ") " + access

	def __get__(self, instance, owner):
		if instance is None:
			return self

		if not self._readable:
			raise AttributeError("unreadable attribute")

		return instance._object["org.freedesktop.DBus.Properties"].Get(self._iface_name, self.__name__)

	def __set__(self, instance, value):
		if instance is None or not self._writeable:
			raise AttributeError("can't set attribute")

		instance._object["org.freedesktop.DBus.Properties"].Set(self._iface_name, self.__name__, GLib.Variant(self._type, value))

	def __repr__(self):
		return "<property " + self.__qualname__ + " at 0x" + format(id(self), "x") + ">"
