# binding.py
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

from dataclasses import dataclass

from .common import *
from .expression import Expression, LiteralExpr, LookupOp


class BindingFlag(AstNode):
    grammar = [
        AnyOf(
            UseExact("flag", "inverted"),
            UseExact("flag", "bidirectional"),
            UseExact("flag", "no-sync-create"),
            UseExact("flag", "sync-create"),
        )
    ]

    @property
    def flag(self) -> str:
        return self.tokens["flag"]

    @validate()
    def sync_create(self):
        if self.flag == "sync-create":
            raise UpgradeWarning(
                "'sync-create' is now the default. Use 'no-sync-create' if this is not wanted.",
                actions=[CodeAction("remove 'sync-create'", "")],
            )

    @validate()
    def unique(self):
        self.validate_unique_in_parent(
            f"Duplicate flag '{self.flag}'", lambda x: x.flag == self.flag
        )

    @validate()
    def flags_only_if_simple(self):
        if self.parent.simple_binding is None:
            raise CompileError(
                "Only bindings with a single lookup can have flags",
            )


class Binding(AstNode):
    grammar = [
        AnyOf(Keyword("bind"), UseExact("bind", "bind-property")),
        Expression,
        ZeroOrMore(BindingFlag),
    ]

    @property
    def expression(self) -> Expression:
        return self.children[Expression][0]

    @property
    def flags(self) -> T.List[BindingFlag]:
        return self.children[BindingFlag]

    @property
    def simple_binding(self) -> T.Optional["SimpleBinding"]:
        if isinstance(self.expression.last, LookupOp):
            if isinstance(self.expression.last.lhs, LiteralExpr):
                from .values import IdentLiteral

                if isinstance(self.expression.last.lhs.literal.value, IdentLiteral):
                    flags = [x.flag for x in self.flags]
                    return SimpleBinding(
                        self.expression.last.lhs.literal.value.ident,
                        self.expression.last.property_name,
                        no_sync_create="no-sync-create" in flags,
                        bidirectional="bidirectional" in flags,
                        inverted="inverted" in flags,
                    )
        return None

    @validate("bind")
    def bind_property(self):
        if self.tokens["bind"] == "bind-property":
            raise UpgradeWarning(
                "'bind-property' is no longer needed. Use 'bind' instead. (blueprint 0.8.2)",
                actions=[CodeAction("use 'bind'", "bind")],
            )


@dataclass
class SimpleBinding:
    source: str
    property_name: str
    no_sync_create: bool = False
    bidirectional: bool = False
    inverted: bool = False
