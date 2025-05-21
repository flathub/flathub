from collections import OrderedDict
from inspect import getargspec

class _empty:
	pass

class Signature:
	empty = _empty

	def __init__(self, parameters=None, return_annotation=_empty):
		self.parameters = OrderedDict(((param.name, param) for param in parameters))
		self.return_annotation = return_annotation

class Parameter:
	empty = _empty

	POSITIONAL_ONLY = 0
	POSITIONAL_OR_KEYWORD = 1
	KEYWORD_ONLY = 999

	def __init__(self, name, kind, default=_empty, annotation=_empty):
		self.name = name
		self.kind = kind
		self.annotation = annotation

def signature(f):
	parameters = [Parameter(arg, Parameter.POSITIONAL_OR_KEYWORD) for arg in getargspec(f).args]
	return Signature(parameters = parameters)
