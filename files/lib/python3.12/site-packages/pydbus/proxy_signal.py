from .generic import bound_signal

class ProxySignal(object):
	def __init__(self, iface_name, signal):
		self._iface_name = iface_name
		self.__name__ = signal.attrib["name"]
		self.__qualname__ = self._iface_name + "." + self.__name__

		self._args = [arg.attrib["type"] for arg in signal if arg.tag == "arg"]
		self.__doc__ = "Signal. Callback: (" + ", ".join(self._args) + ")"

	def connect(self, object, callback):
		"""Subscribe to the signal."""
		def signal_fired(sender, object, iface, signal, params):
			callback(*params)
		return object._bus.subscribe(sender=object._bus_name, object=object._path, iface=self._iface_name, signal=self.__name__, signal_fired=signal_fired)

	def __get__(self, instance, owner):
		if instance is None:
			return self

		return bound_signal(self, instance)

	def __set__(self, instance, value):
		raise AttributeError("can't set attribute")

	def __repr__(self):
		return "<signal " + self.__qualname__ + " at 0x" + format(id(self), "x") + ">"

class OnSignal(object):
	def __init__(self, signal):
		self.signal = signal
		self.__name__ = "on" + signal.__name__
		self.__qualname__ = signal._iface_name + "." + self.__name__
		self.__doc__ = "Assign a callback to subscribe to the signal. Assing None to unsubscribe. Callback: (" + ", ".join(signal._args) + ")"

	def __get__(self, instance, owner):
		if instance is None:
			return self

		try:
			return getattr(instance, "_on" + self.signal.__name__)
		except AttributeError:
			return None

	def __set__(self, instance, value):
		if instance is None:
			raise AttributeError("can't set attribute")

		try:
			old = getattr(instance, "_sub" + self.signal.__name__)
			old.unsubscribe()
		except AttributeError:
			pass

		if value is None:
			delattr(instance, "_on" + self.signal.__name__)
			delattr(instance, "_sub" + self.signal.__name__)
			return

		sub = self.signal.connect(instance, value)
		setattr(instance, "_on" + self.signal.__name__, value)
		setattr(instance, "_sub" + self.signal.__name__, sub)

	def __repr__(self):
		return "<descriptor " + self.__qualname__ + " at 0x" + format(id(self), "x") + ">"
