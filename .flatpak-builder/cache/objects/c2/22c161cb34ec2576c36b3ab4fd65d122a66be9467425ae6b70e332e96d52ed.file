# gtk_styles.py
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


class StyleClass(AstNode):
    grammar = UseQuoted("name")

    @property
    def name(self) -> str:
        return self.tokens["name"]

    @validate("name")
    def unique_in_parent(self):
        self.validate_unique_in_parent(
            f"Duplicate style class '{self.name}'", lambda x: x.name == self.name
        )


class ExtStyles(AstNode):
    grammar = [
        Keyword("styles"),
        "[",
        Delimited(StyleClass, ","),
        "]",
    ]

    @validate("styles")
    def container_is_widget(self):
        validate_parent_type(self, "Gtk", "Widget", "style classes")

    @validate("styles")
    def unique_in_parent(self):
        self.validate_unique_in_parent("Duplicate styles block")


@completer(
    applies_in=[ObjectContent],
    applies_in_subclass=("Gtk", "Widget"),
    matches=new_statement_patterns,
)
def style_completer(ast_node, match_variables):
    yield Completion("styles", CompletionItemKind.Keyword, snippet='styles ["$0"]')


@decompiler("style")
def decompile_style(ctx, gir):
    ctx.print(f"styles [")


@decompiler("class")
def decompile_style_class(ctx, gir, name):
    ctx.print(f'"{name}",')
