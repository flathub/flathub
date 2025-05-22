from gi.repository import Gio

def auto_bus_name(bus_name):
	if bus_name[0] == ".":
		#Default namespace
		bus_name = "org.freedesktop" + bus_name

	if not Gio.dbus_is_name(bus_name):
		raise ValueError("invalid bus name")

	return bus_name

def auto_object_path(bus_name, object_path=None):
	if object_path is None:
		# They always name it like that.
		object_path = "/" + bus_name.replace(".", "/")

	if object_path[0] != "/":
		object_path = "/" + bus_name.replace(".", "/") + "/" + object_path

	return object_path
