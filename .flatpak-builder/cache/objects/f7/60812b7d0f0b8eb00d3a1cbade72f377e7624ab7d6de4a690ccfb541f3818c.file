# gtk_size_group.py
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
from .contexts import ScopeCtx
from .gobject_object import ObjectContent, validate_parent_type


class Widget(AstNode):
    grammar = UseIdent("name")

    @property
    def name(self) -> str:
        return self.tokens["name"]

    @validate("name")
    def obj_widget(self):
        object = self.context[ScopeCtx].objects.get(self.tokens["name"])
        type = self.root.gir.get_type("Widget", "Gtk")
        if object is None:
            raise CompileError(
                f"Could not find object with ID {self.tokens['name']}",
                did_you_mean=(
                    self.tokens["name"],
                    self.context[ScopeCtx].objects.keys(),
                ),
            )
        elif object.gir_class and not object.gir_class.assignable_to(type):
            raise CompileError(
                f"Cannot assign {object.gir_class.full_name} to {type.full_name}"
            )

    @validate("name")
    def unique_in_parent(self):
        self.validate_unique_in_parent(
            f"Object '{self.name}' is listed twice", lambda x: x.name == self.name
        )


class ExtSizeGroupWidgets(AstNode):
    grammar = [
        Keyword("widgets"),
        "[",
        Delimited(Widget, ","),
        "]",
    ]

    @validate("widgets")
    def container_is_size_group(self):
        validate_parent_type(self, "Gtk", "SizeGroup", "size group properties")

    @validate("widgets")
    def unique_in_parent(self):
        self.validate_unique_in_parent("Duplicate widgets block")


@completer(
    applies_in=[ObjectContent],
    applies_in_subclass=("Gtk", "SizeGroup"),
    matches=new_statement_patterns,
)
def size_group_completer(ast_node, match_variables):
    yield Completion("widgets", CompletionItemKind.Snippet, snippet="widgets [$0]")
