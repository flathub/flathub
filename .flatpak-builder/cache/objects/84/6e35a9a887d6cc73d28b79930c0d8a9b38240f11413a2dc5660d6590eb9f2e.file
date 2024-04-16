# typelib.py
#
# Copyright 2022 James Westman <james@jwestman.net>
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

import math
import mmap
import os
import sys
import typing as T
from ctypes import *

from .errors import CompilerBugError

BLOB_TYPE_STRUCT = 3
BLOB_TYPE_BOXED = 4
BLOB_TYPE_ENUM = 5
BLOB_TYPE_FLAGS = 6
BLOB_TYPE_OBJECT = 7
BLOB_TYPE_INTERFACE = 8

TYPE_VOID = 0
TYPE_BOOLEAN = 1
TYPE_INT8 = 2
TYPE_UINT8 = 3
TYPE_INT16 = 4
TYPE_UINT16 = 5
TYPE_INT32 = 6
TYPE_UINT32 = 7
TYPE_INT64 = 8
TYPE_UINT64 = 9
TYPE_FLOAT = 10
TYPE_DOUBLE = 11
TYPE_GTYPE = 12
TYPE_UTF8 = 13
TYPE_FILENAME = 14
TYPE_ARRAY = 15
TYPE_INTERFACE = 16
TYPE_GLIST = 17
TYPE_GSLIST = 18
TYPE_GHASH = 19
TYPE_ERROR = 20
TYPE_UNICHAR = 21


class Field:
    def __init__(self, offset: int, type: str, shift=0, mask=None):
        self._offset = offset
        self._type = type
        self._shift = shift
        self._mask = (1 << mask) - 1 if mask else None
        self._name = f"{offset}__{type}__{shift}__{mask}"

    def __get__(self, typelib: "Typelib", _objtype=None):
        if typelib is None:
            return self

        def shift_mask(n):
            n = n >> self._shift
            if self._mask:
                n = n & self._mask
            return n

        tl = typelib[self._offset]
        if self._type == "u8":
            return shift_mask(tl.u8)
        elif self._type == "u16":
            return shift_mask(tl.u16)
        elif self._type == "u32":
            return shift_mask(tl.u32)
        elif self._type == "i8":
            return shift_mask(tl.i8)
        elif self._type == "i16":
            return shift_mask(tl.i16)
        elif self._type == "i32":
            return shift_mask(tl.i32)
        elif self._type == "pointer":
            return tl.header[tl.u32]
        elif self._type == "offset":
            return tl
        elif self._type == "string":
            return tl.string
        elif self._type == "dir_entry":
            return tl.header.dir_entry(tl.u16)
        else:
            raise CompilerBugError(self._type)


class Typelib:
    AS_DIR_ENTRY = Field(0, "dir_entry")

    HEADER_N_ENTRIES = Field(0x14, "u16")
    HEADER_N_LOCAL_ENTRIES = Field(0x16, "u16")
    HEADER_DIRECTORY = Field(0x18, "pointer")
    HEADER_N_ATTRIBUTES = Field(0x1C, "u32")
    HEADER_ATTRIBUTES = Field(0x20, "pointer")

    HEADER_DEPENDENCIES = Field(0x24, "pointer")

    HEADER_NAMESPACE = Field(0x2C, "string")
    HEADER_NSVERSION = Field(0x30, "string")

    HEADER_ENTRY_BLOB_SIZE = Field(0x3C, "u16")
    HEADER_FUNCTION_BLOB_SIZE = Field(0x3E, "u16")
    HEADER_CALLBACK_BLOB_SIZE = Field(0x40, "u16")
    HEADER_SIGNAL_BLOB_SIZE = Field(0x42, "u16")
    HEADER_ARG_BLOB_SIZE = Field(0x46, "u16")
    HEADER_PROPERTY_BLOB_SIZE = Field(0x48, "u16")
    HEADER_FIELD_BLOB_SIZE = Field(0x4A, "u16")
    HEADER_VALUE_BLOB_SIZE = Field(0x4C, "u16")
    HEADER_ATTRIBUTE_BLOB_SIZE = Field(0x4E, "u16")
    HEADER_ENUM_BLOB_SIZE = Field(0x56, "u16")
    HEADER_OBJECT_BLOB_SIZE = Field(0x5A, "u16")
    HEADER_INTERFACE_BLOB_SIZE = Field(0x5C, "u16")

    DIR_ENTRY_BLOB_TYPE = Field(0x0, "u16")
    DIR_ENTRY_LOCAL = Field(0x2, "u16", 0, 1)
    DIR_ENTRY_NAME = Field(0x4, "string")
    DIR_ENTRY_OFFSET = Field(0x8, "pointer")
    DIR_ENTRY_NAMESPACE = Field(0x8, "string")

    ARG_NAME = Field(0x0, "string")
    ARG_TYPE = Field(0xC, "u32")

    SIGNATURE_RETURN_TYPE = Field(0x0, "u32")
    SIGNATURE_N_ARGUMENTS = Field(0x6, "u16")
    SIGNATURE_ARGUMENTS = Field(0x8, "offset")

    ATTR_OFFSET = Field(0x0, "u32")
    ATTR_NAME = Field(0x0, "string")
    ATTR_VALUE = Field(0x0, "string")

    TYPE_BLOB_TAG = Field(0x0, "u8", 3, 5)
    TYPE_BLOB_INTERFACE = Field(0x2, "dir_entry")
    TYPE_BLOB_ARRAY_INNER = Field(0x4, "u32")

    BLOB_NAME = Field(0x4, "string")

    ENUM_GTYPE_NAME = Field(0x8, "string")
    ENUM_N_VALUES = Field(0x10, "u16")
    ENUM_N_METHODS = Field(0x12, "u16")
    ENUM_VALUES = Field(0x18, "offset")

    INTERFACE_GTYPE_NAME = Field(0x8, "string")
    INTERFACE_N_PREREQUISITES = Field(0x12, "u16")
    INTERFACE_N_PROPERTIES = Field(0x14, "u16")
    INTERFACE_N_METHODS = Field(0x16, "u16")
    INTERFACE_N_SIGNALS = Field(0x18, "u16")
    INTERFACE_N_VFUNCS = Field(0x1A, "u16")
    INTERFACE_N_CONSTANTS = Field(0x1C, "u16")
    INTERFACE_PREREQUISITES = Field(0x28, "offset")

    OBJ_DEPRECATED = Field(0x02, "u16", 0, 1)
    OBJ_ABSTRACT = Field(0x02, "u16", 1, 1)
    OBJ_FUNDAMENTAL = Field(0x02, "u16", 2, 1)
    OBJ_FINAL = Field(0x02, "u16", 3, 1)
    OBJ_GTYPE_NAME = Field(0x08, "string")
    OBJ_PARENT = Field(0x10, "dir_entry")
    OBJ_GTYPE_STRUCT = Field(0x14, "string")
    OBJ_N_INTERFACES = Field(0x14, "u16")
    OBJ_N_FIELDS = Field(0x16, "u16")
    OBJ_N_PROPERTIES = Field(0x18, "u16")
    OBJ_N_METHODS = Field(0x1A, "u16")
    OBJ_N_SIGNALS = Field(0x1C, "u16")
    OBJ_N_VFUNCS = Field(0x1E, "u16")
    OBJ_N_CONSTANTS = Field(0x20, "u16")
    OBJ_N_FIELD_CALLBACKS = Field(0x22, "u16")

    PROP_NAME = Field(0x0, "string")
    PROP_DEPRECATED = Field(0x4, "u32", 0, 1)
    PROP_READABLE = Field(0x4, "u32", 1, 1)
    PROP_WRITABLE = Field(0x4, "u32", 2, 1)
    PROP_CONSTRUCT = Field(0x4, "u32", 3, 1)
    PROP_CONSTRUCT_ONLY = Field(0x4, "u32", 4, 1)
    PROP_TYPE = Field(0xC, "u32")

    SIGNAL_DEPRECATED = Field(0x0, "u16", 0, 1)
    SIGNAL_DETAILED = Field(0x0, "u16", 5, 1)
    SIGNAL_NAME = Field(0x4, "string")
    SIGNAL_SIGNATURE = Field(0xC, "pointer")

    VALUE_NAME = Field(0x4, "string")
    VALUE_VALUE = Field(0x8, "i32")

    def __init__(self, typelib_file, offset: int):
        self._typelib_file = typelib_file
        self._offset = offset

    def __getitem__(self, index: int):
        return Typelib(self._typelib_file, self._offset + index)

    def attr(self, name):
        return self.header.attr(self._offset, name)

    @property
    def header(self) -> "TypelibHeader":
        return TypelibHeader(self._typelib_file)

    @property
    def u8(self) -> int:
        """Gets the 8-bit unsigned int at this location."""
        return self._int(1, False)

    @property
    def u16(self) -> int:
        """Gets the 16-bit unsigned int at this location."""
        return self._int(2, False)

    @property
    def u32(self) -> int:
        """Gets the 32-bit unsigned int at this location."""
        return self._int(4, False)

    @property
    def i8(self) -> int:
        """Gets the 8-bit unsigned int at this location."""
        return self._int(1, True)

    @property
    def i16(self) -> int:
        """Gets the 16-bit unsigned int at this location."""
        return self._int(2, True)

    @property
    def i32(self) -> int:
        """Gets the 32-bit unsigned int at this location."""
        return self._int(4, True)

    @property
    def string(self) -> T.Optional[str]:
        """Interprets the 32-bit unsigned int at this location as a pointer
        within the typelib file, and returns the null-terminated string at that
        pointer."""

        loc = self.u32
        if loc == 0:
            return None

        end = self._typelib_file.find(b"\0", loc)
        return self._typelib_file[loc:end].decode("utf-8")

    def _int(self, size, signed) -> int:
        return int.from_bytes(
            self._typelib_file[self._offset : self._offset + size], sys.byteorder
        )


class TypelibHeader(Typelib):
    def __init__(self, typelib_file):
        super().__init__(typelib_file, 0)

    def dir_entry(self, index) -> T.Optional[Typelib]:
        if index == 0:
            return None
        else:
            return self.HEADER_DIRECTORY[(index - 1) * self.HEADER_ENTRY_BLOB_SIZE]

    def attr(self, offset, name):
        lower = 0
        upper = self.HEADER_N_ATTRIBUTES
        attr_size = self.HEADER_ATTRIBUTE_BLOB_SIZE
        attrs = self.HEADER_ATTRIBUTES
        mid = 0

        while lower <= upper:
            mid = math.floor((upper + lower) / 2)
            attr = attrs[mid * attr_size]
            if attr.ATTR_OFFSET < offset:
                lower = mid + 1
            elif attr.ATTR_OFFSET > offset:
                upper = mid - 1
            else:
                while mid >= 0 and attrs[(mid - 1) * attr_size].ATTR_OFFSET == offset:
                    mid -= 1
                break
        if attrs[mid * attr_size].ATTR_OFFSET != offset:
            # no match found
            return None
        while attrs[mid * attr_size].ATTR_OFFSET == offset:
            if attrs[mid * attr_size].ATTR_NAME == name:
                return attrs[mid * attr_size].ATTR_VALUE
            mid += 1
        return None

    def attr_by_index(self, index):
        pass

    @property
    def dir_entries(self):
        return [self.dir_entry(i) for i in range(self[0x16].u16)]


def load_typelib(path: str) -> Typelib:
    with open(path, "rb") as f:
        return Typelib(f.read(), 0)
