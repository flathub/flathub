from .exitable import ExitableWithAliases

class NameOwner(ExitableWithAliases("unown")):
	__slots__ = ()

	def __init__(self, bus, name, allow_replacement, replace):
		flags = 4 | (1 if allow_replacement else 0) | (2 if replace else 0)
		res = bus.dbus.RequestName(name, flags)
		if res == 1:
			self._at_exit(lambda: bus.dbus.ReleaseName(name))
			return # OK
		if res == 3:
			raise RuntimeError("name already exists on the bus")
		if res == 4:
			raise RuntimeError("you're already the owner of this name")
		raise RuntimeError("cannot take ownership of the name")

class RequestNameMixin(object):
	__slots__ = ()

	def request_name(self, name, allow_replacement=True, replace=False):
		"""Aquires a bus name.

		Returns
		-------
		NameOwner
			An object you can use as a context manager to unown the name later.
		"""
		return NameOwner(self, name, allow_replacement, replace)
