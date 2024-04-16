# gtk_scale.py
#
# Copyright 2023 James Westman <james@jwestman.net>
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


class ExtScaleMark(AstNode):
    grammar = [
        Keyword("mark"),
        Match("(").expected(),
        [
            Optional(AnyOf(UseExact("sign", "-"), UseExact("sign", "+"))),
            UseNumber("value"),
            Optional(
                [
                    ",",
                    UseIdent("position"),
                    Optional([",", StringValue]),
                ]
            ),
        ],
        Match(")").expected(),
    ]

    @property
    def value(self) -> float:
        if self.tokens["sign"] == "-":
            return -self.tokens["value"]
        else:
            return self.tokens["value"]

    @property
    def position(self) -> T.Optional[str]:
        return self.tokens["position"]

    @property
    def label(self) -> T.Optional[StringValue]:
        if len(self.children[StringValue]) == 1:
            return self.children[StringValue][0]
        else:
            return None

    @docs("position")
    def position_docs(self) -> T.Optional[str]:
        if member := self.root.gir.get_type("PositionType", "Gtk").members.get(
            self.position
        ):
            return member.doc
        else:
            return None

    @validate("position")
    def validate_position(self):
        positions = self.root.gir.get_type("PositionType", "Gtk").members
        if self.position is not None and positions.get(self.position) is None:
            raise CompileError(
                f"'{self.position}' is not a member of Gtk.PositionType",
                did_you_mean=(self.position, positions.keys()),
            )


class ExtScaleMarks(AstNode):
    grammar = [
        Keyword("marks"),
        Match("[").expected(),
        Until(ExtScaleMark, "]", ","),
    ]

    @property
    def marks(self) -> T.List[ExtScaleMark]:
        return self.children

    @validate("marks")
    def container_is_size_group(self):
        validate_parent_type(self, "Gtk", "Scale", "scale marks")

    @validate("marks")
    def unique_in_parent(self):
        self.validate_unique_in_parent("Duplicate 'marks' block")


@completer(
    applies_in=[ObjectContent],
    applies_in_subclass=("Gtk", "Scale"),
    matches=new_statement_patterns,
)
def complete_marks(ast_node, match_variables):
    yield Completion("marks", CompletionItemKind.Keyword, snippet="marks [\n\t$0\n]")


@completer(
    applies_in=[ExtScaleMarks],
)
def complete_mark(ast_node, match_variables):
    yield Completion("mark", CompletionItemKind.Keyword, snippet="mark ($0),")


@decompiler("marks")
def decompile_marks(
    ctx,
    gir,
):
    ctx.print("marks [")


@decompiler("mark", cdata=True)
def decompile_mark(
    ctx: DecompileCtx,
    gir,
    value,
    position=None,
    cdata=None,
    translatable="false",
    comments=None,
    context=None,
):
    if comments is not None:
        ctx.print(f"/* Translators: {comments} */")

    text = f"mark ({value}"

    if position:
        text += f", {position}"
    elif cdata:
        text += f", bottom"

    if truthy(translatable):
        comments, translatable = decompile_translatable(
            cdata, translatable, context, comments
        )
        text += f", {translatable}"

    text += "),"
    ctx.print(text)
