# xml_reader.py
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
from collections import defaultdict
from functools import cached_property
from xml import sax

# To speed up parsing, we ignore all tags except these
PARSE_GIR = set(
    [
        "repository",
        "namespace",
        "class",
        "interface",
        "property",
        "glib:signal",
        "include",
        "implements",
        "type",
        "parameter",
        "parameters",
        "enumeration",
        "member",
        "bitfield",
    ]
)


class Element:
    def __init__(self, tag: str, attrs: T.Dict[str, str]):
        self.tag = tag
        self.attrs = attrs
        self.children: T.List["Element"] = []
        self.cdata_chunks: T.List[str] = []

    @cached_property
    def cdata(self):
        return "".join(self.cdata_chunks)

    def get_elements(self, name: str) -> T.List["Element"]:
        return [child for child in self.children if child.tag == name]

    def __getitem__(self, key: str):
        return self.attrs.get(key)


class Handler(sax.handler.ContentHandler):
    def __init__(self):
        self.root = None
        self.stack = []

    def startElement(self, name, attrs):
        element = Element(name, attrs.copy())

        if len(self.stack):
            last = self.stack[-1]
            last.children.append(element)
        else:
            self.root = element

        self.stack.append(element)

    def endElement(self, name):
        self.stack.pop()

    def characters(self, content):
        self.stack[-1].cdata_chunks.append(content)


def parse(filename):
    parser = sax.make_parser()
    handler = Handler()
    parser.setContentHandler(handler)
    parser.parse(filename)
    return handler.root


def parse_string(xml):
    handler = Handler()
    parser = sax.parseString(xml, handler)
    return handler.root
