# gtk_combo_box_text.py
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


from .common import *
from .gobject_object import ObjectContent, validate_parent_type
from .values import StringValue


class Item(AstNode):
    grammar = StringValue

    @property
    def child(self) -> StringValue:
        return self.children[StringValue][0]


class ExtStringListStrings(AstNode):
    grammar = [
        Keyword("strings"),
        "[",
        Delimited(Item, ","),
        "]",
    ]

    @validate("items")
    def container_is_string_list(self):
        validate_parent_type(self, "Gtk", "StringList", "StringList items")

    @validate("strings")
    def unique_in_parent(self):
        self.validate_unique_in_parent("Duplicate strings block")


@completer(
    applies_in=[ObjectContent],
    applies_in_subclass=("Gtk", "StringList"),
    matches=new_statement_patterns,
)
def strings_completer(ast_node, match_variables):
    yield Completion("strings", CompletionItemKind.Snippet, snippet="strings [$0]")
