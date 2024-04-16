# gtk_layout.py
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
from .contexts import ValueTypeCtx
from .gobject_object import ObjectContent, validate_parent_type
from .values import Value


class LayoutProperty(AstNode):
    grammar = Statement(UseIdent("name"), ":", Err(Value, "Expected a value"))
    tag_name = "property"

    @property
    def name(self) -> str:
        return self.tokens["name"]

    @property
    def value(self) -> Value:
        return self.children[Value][0]

    @context(ValueTypeCtx)
    def value_type(self) -> ValueTypeCtx:
        # there isn't really a way to validate these
        return ValueTypeCtx(None)

    @validate("name")
    def unique_in_parent(self):
        self.validate_unique_in_parent(
            f"Duplicate layout property '{self.name}'",
            check=lambda child: child.name == self.name,
        )


class ExtLayout(AstNode):
    grammar = Sequence(
        Keyword("layout"),
        "{",
        Until(LayoutProperty, "}"),
    )

    @validate("layout")
    def container_is_widget(self):
        validate_parent_type(self, "Gtk", "Widget", "layout properties")

    @validate("layout")
    def unique_in_parent(self):
        self.validate_unique_in_parent("Duplicate layout block")


@completer(
    applies_in=[ObjectContent],
    applies_in_subclass=("Gtk", "Widget"),
    matches=new_statement_patterns,
)
def layout_completer(ast_node, match_variables):
    yield Completion("layout", CompletionItemKind.Snippet, snippet="layout {\n  $0\n}")


@decompiler("layout")
def decompile_layout(ctx, gir):
    ctx.print("layout {")
