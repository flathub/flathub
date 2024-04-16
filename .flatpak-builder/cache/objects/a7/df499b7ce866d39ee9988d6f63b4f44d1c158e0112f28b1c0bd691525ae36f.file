# gobject_signal.py
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

import typing as T

from .common import *
from .contexts import ScopeCtx
from .gtkbuilder_template import Template


class SignalFlag(AstNode):
    grammar = AnyOf(
        UseExact("flag", "swapped"),
        UseExact("flag", "after"),
    )

    @property
    def flag(self) -> str:
        return self.tokens["flag"]

    @validate()
    def unique(self):
        self.validate_unique_in_parent(
            f"Duplicate flag '{self.flag}'", lambda x: x.flag == self.flag
        )


class Signal(AstNode):
    grammar = Statement(
        UseIdent("name"),
        Optional(
            [
                "::",
                UseIdent("detail_name").expected("a signal detail name"),
            ]
        ),
        "=>",
        Optional(["$", UseLiteral("extern", True)]),
        UseIdent("handler").expected("the name of a function to handle the signal"),
        Match("(").expected("argument list"),
        Optional(UseIdent("object")).expected("object identifier"),
        Match(")").expected(),
        ZeroOrMore(SignalFlag),
    )

    @property
    def name(self) -> str:
        return self.tokens["name"]

    @property
    def detail_name(self) -> T.Optional[str]:
        return self.tokens["detail_name"]

    @property
    def full_name(self) -> str:
        if self.detail_name is None:
            return self.name
        else:
            return self.name + "::" + self.detail_name

    @property
    def handler(self) -> str:
        return self.tokens["handler"]

    @property
    def object_id(self) -> T.Optional[str]:
        return self.tokens["object"]

    @property
    def flags(self) -> T.List[SignalFlag]:
        return self.children[SignalFlag]

    @property
    def is_swapped(self) -> bool:
        return any(x.flag == "swapped" for x in self.flags)

    @property
    def is_after(self) -> bool:
        return any(x.flag == "after" for x in self.flags)

    @property
    def gir_signal(self):
        if self.gir_class is not None and not isinstance(self.gir_class, ExternType):
            return self.gir_class.signals.get(self.tokens["name"])

    @property
    def gir_class(self):
        return self.parent.parent.gir_class

    @validate("handler")
    def old_extern(self):
        if not self.tokens["extern"]:
            if self.handler is not None:
                raise UpgradeWarning(
                    "Use the '$' extern syntax introduced in blueprint 0.8.0",
                    actions=[CodeAction("Use '$' syntax", "$" + self.handler)],
                )

    @validate("name")
    def signal_exists(self):
        if self.gir_class is None or self.gir_class.incomplete:
            # Objects that we have no gir data on should not be validated
            # This happens for classes defined by the app itself
            return

        if self.gir_signal is None:
            raise CompileError(
                f"Class {self.gir_class.full_name} does not contain a signal called {self.tokens['name']}",
                did_you_mean=(self.tokens["name"], self.gir_class.signals.keys()),
            )

    @validate("object")
    def object_exists(self):
        object_id = self.tokens["object"]
        if object_id is None:
            return

        if self.context[ScopeCtx].objects.get(object_id) is None:
            raise CompileError(f"Could not find object with ID '{object_id}'")

    @docs("name")
    def signal_docs(self):
        if self.gir_signal is not None:
            return self.gir_signal.doc


@decompiler("signal")
def decompile_signal(ctx, gir, name, handler, swapped="false", object=None):
    object_name = object or ""
    name = name.replace("_", "-")
    if decompile.truthy(swapped):
        ctx.print(f"{name} => ${handler}({object_name}) swapped;")
    else:
        ctx.print(f"{name} => ${handler}({object_name});")
    return gir
