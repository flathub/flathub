import inspect

class Exitable(object):
	__slots__ = ("_at_exit_cbs")

	def _at_exit(self, cb):
		try:
			self._at_exit_cbs
		except AttributeError:
			self._at_exit_cbs = []

		self._at_exit_cbs.append(cb)

	def __enter__(self):
		return self

	def __exit__(self, exc_type = None, exc_value = None, traceback = None):
		if self._exited:
			return

		for cb in reversed(self._at_exit_cbs):
			call_with_exc = True
			try:
				inspect.getcallargs(cb, exc_type, exc_value, traceback)
			except TypeError:
				call_with_exc = False

			if call_with_exc:
				cb(exc_type, exc_value, traceback)
			else:
				cb()

		self._at_exit_cbs = None

	@property
	def _exited(self):
		try:
			return self._at_exit_cbs is None
		except AttributeError:
			return True

def ExitableWithAliases(*exit_methods):
	class CustomExitable(Exitable):
		pass

	def exit(self):
		self.__exit__()

	for exit_method_name in exit_methods:
		setattr(CustomExitable, exit_method_name, exit)

	return CustomExitable
