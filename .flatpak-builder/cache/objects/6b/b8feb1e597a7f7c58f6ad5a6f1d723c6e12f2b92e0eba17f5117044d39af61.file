# -*- Mode: Python -*-
# GObject-Introspection - a framework for introspecting GObject libraries
# Copyright (C) 2008  Johan Dahlin
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import imp
import os
import sys

from .utils import extract_libtool


class LibtoolImporter(object):

    def __init__(self, name, path):
        self.name = name
        self.path = path

    @classmethod
    def find_module(cls, name, packagepath=None):
        modparts = name.split('.')
        filename = modparts.pop() + '.la'

        # Given some.package.module 'path' is where subpackages of some.package
        # should be looked for. See if we can find a ".libs/module.la" relative
        # to those directories and failing that look for file
        # "some/package/.libs/module.la" relative to sys.path
        if len(modparts) > 0:
            modprefix = os.path.join(*modparts)
            modprefix = os.path.join(modprefix, '.libs')
        else:
            modprefix = '.libs'

        for path in sys.path:
            full = os.path.join(path, modprefix, filename)
            if os.path.exists(full):
                return cls(name, full)

    def load_module(self, name):
        realpath = extract_libtool(self.path)

        # The first item of the suffix tuple (which can be, depending on platform,
        # one or more valid filename extensions used to name c extension modules)
        # is ignored by imp.load_module(). Thus, there is no use in pretending it
        # is important and we set it to an empty string.
        suffix = ('', 'rb', imp.C_EXTENSION)

        mod = imp.load_module(name, open(realpath), realpath, suffix)
        mod.__loader__ = self
        return mod

    @classmethod
    def __enter__(cls):
        sys.meta_path.append(cls)

    @classmethod
    def __exit__(cls, exc_type, exc_val, exc_tb):
        sys.meta_path.remove(cls)
