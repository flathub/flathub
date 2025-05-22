from gi.repository import Gio
from .exitable import ExitableWithAliases
from .auto_names import *

class Publication(ExitableWithAliases("unpublish")):
	__slots__ = ()

	def __init__(self, bus, bus_name, *objects, **kwargs): # allow_replacement=True, replace=False
		# Python 2 sux
		for kwarg in kwargs:
			if kwarg not in ("allow_replacement", "replace",):
				raise TypeError(self.__qualname__ + " got an unexpected keyword argument '{}'".format(kwarg))
		allow_replacement = kwargs.get("allow_replacement", True)
		replace = kwargs.get("replace", False)

		bus_name = auto_bus_name(bus_name)

		for object_info in objects:
			path, object, node_info = (None, None, None)

			if type(object_info) == tuple:
				if len(object_info) == 3:
					path, object, node_info = object_info
				if len(object_info) == 2:
					path, object = object_info
				if len(object_info) == 1:
					object = object_info[0]
			else:
				object = object_info

			path = auto_object_path(bus_name, path)
			self._at_exit(bus.register_object(path, object, node_info).__exit__)

		# Request name only after registering all the objects.
		self._at_exit(bus.request_name(bus_name, allow_replacement=allow_replacement, replace=replace).__exit__)

class PublicationMixin(object):
	__slots__ = ()

	def publish(self, bus_name, *objects):
		"""Expose objects on the bus."""
		return Publication(self, bus_name, *objects)
