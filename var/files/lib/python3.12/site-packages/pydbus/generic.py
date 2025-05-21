"""Generic programming utilities.

Utilities implemented in this file are not dependent
on dbus, they can be used everywhere.
"""

class subscription(object):
	__slots__ = ("callback_list", "callback")

	def __init__(self, callback_list, callback):
		self.callback_list = callback_list
		self.callback = callback
		self.callback_list.append(callback)

	def unsubscribe(self):
		self.callback_list.remove(self.callback)
		self.callback_list = None
		self.callback = None

	def disconnect(self):
		"""An alias for unsubscribe()"""
		self.unsubscribe()

	def __enter__(self):
		return self

	def __exit__(self, exc_type, exc_value, traceback):
		if not self.callback is None:
			self.unsubscribe()

class bound_signal(object):
	__slots__ = ("__signal__", "__self__") # bound method uses ("__func__", "__self__")

	def __init__(self, signal, instance):
		self.__signal__ = signal
		self.__self__ = instance

	@property
	def callbacks(self):
		return self.__signal__.map[self.__self__]

	def connect(self, callback):
		"""Subscribe to the signal."""
		return self.__signal__.connect(self.__self__, callback)

	def emit(self, *args):
		"""Emit the signal."""
		self.__signal__.emit(self.__self__, *args)

	def __call__(self, *args):
		"""Emit the signal."""
		self.emit(*args)

	def __repr__(self):
		return "<bound signal " + self.__signal__.__qualname__ + " of " + repr(self.__self__) + ">"

class signal(object):
	"""Static signal object

	You're expected to set it as a class property::

		class A:
			SomethingHappened = signal()

	Declared this way, it can be used on class instances
	to connect signal observers::

		a = A()
		a.SomethingHappened.connect(func)

	and emit the signal::

		a.SomethingHappened()

	You may pass any parameters to the emiting function
	- they will be forwarded to all subscribed callbacks.
	"""

	def __init__(self):
		self.map = {}
		self.__qualname__ = "<anonymous signal>" # function uses <lambda> ;)
		self.__doc__ = "Signal."

	def connect(self, object, callback):
		"""Subscribe to the signal."""
		return subscription(self.map.setdefault(object, []), callback)

	def emit(self, object, *args):
		"""Emit the signal."""
		for cb in self.map.get(object, []):
			cb(*args)

	def __get__(self, instance, owner):
		if instance is None:
			return self

		return bound_signal(self, instance)

	def __set__(self, instance, value):
		raise AttributeError("can't set attribute")

	def __repr__(self):
		return "<signal " + self.__qualname__ + " at 0x" + format(id(self), "x") + ">"

bound_method = type(signal().emit) # TODO find a prettier way to get this type
