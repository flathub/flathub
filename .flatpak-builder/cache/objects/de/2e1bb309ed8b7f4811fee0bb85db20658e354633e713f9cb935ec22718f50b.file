# xml_emitter.py
#
# Copyright 2021 James Westman <james@jwestman.net>
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

import typing as T
from xml.sax import saxutils

from blueprintcompiler.gir import GirType
from blueprintcompiler.language.types import ClassName


class XmlEmitter:
    def __init__(self, indent=2):
        self.indent = indent
        self.result = '<?xml version="1.0" encoding="UTF-8"?>'
        self._tag_stack = []
        self._needs_newline = False

    def start_tag(self, tag, **attrs: T.Union[str, GirType, ClassName, bool, None]):
        self._indent()
        self.result += f"<{tag}"
        for key, val in attrs.items():
            if val is not None:
                self.result += f' {key.replace("_", "-")}="{saxutils.escape(self._to_string(val))}"'
        self.result += ">"
        self._tag_stack.append(tag)
        self._needs_newline = False

    def put_self_closing(self, tag, **attrs):
        self._indent()
        self.result += f"<{tag}"
        for key, val in attrs.items():
            if val is not None:
                self.result += f' {key.replace("_", "-")}="{saxutils.escape(self._to_string(val))}"'
        self.result += "/>"
        self._needs_newline = True

    def end_tag(self):
        tag = self._tag_stack.pop()
        if self._needs_newline:
            self._indent()
        self.result += f"</{tag}>"
        self._needs_newline = True

    def put_text(self, text: T.Union[str, int, float]):
        self.result += saxutils.escape(str(text))
        self._needs_newline = False

    def put_cdata(self, text: str):
        self.result += f"<![CDATA[{text}]]>"
        self._needs_newline = False

    def _indent(self):
        if self.indent is not None:
            self.result += "\n" + " " * (self.indent * len(self._tag_stack))

    def _to_string(self, val):
        if isinstance(val, GirType):
            return val.glib_type_name
        elif isinstance(val, ClassName):
            return val.glib_type_name
        else:
            return str(val)
