# types.py
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


from ..gir import Class, ExternType, Interface
from .common import *


class TypeName(AstNode):
    grammar = AnyOf(
        [
            UseIdent("namespace"),
            ".",
            UseIdent("class_name"),
        ],
        [
            AnyOf("$", [".", UseLiteral("old_extern", True)]),
            UseIdent("class_name"),
            UseLiteral("extern", True),
        ],
        UseIdent("class_name"),
    )

    @validate()
    def old_extern(self):
        if self.tokens["old_extern"]:
            raise UpgradeWarning(
                "Use the '$' extern syntax introduced in blueprint 0.8.0",
                actions=[CodeAction("Use '$' syntax", "$" + self.tokens["class_name"])],
            )

    @validate("class_name")
    def type_exists(self):
        if not self.tokens["extern"] and self.gir_ns is not None:
            self.root.gir.validate_type(
                self.tokens["class_name"], self.tokens["namespace"]
            )

    @validate("namespace")
    def gir_ns_exists(self):
        if not self.tokens["extern"]:
            self.root.gir.validate_ns(self.tokens["namespace"])

    @property
    def gir_ns(self):
        if not self.tokens["extern"]:
            return self.root.gir.namespaces.get(self.tokens["namespace"] or "Gtk")

    @property
    def gir_type(self) -> gir.GirType:
        if self.tokens["class_name"] and not self.tokens["extern"]:
            return self.root.gir.get_type(
                self.tokens["class_name"], self.tokens["namespace"]
            )

        return gir.ExternType(self.tokens["class_name"])

    @property
    def glib_type_name(self) -> str:
        if gir_type := self.gir_type:
            return gir_type.glib_type_name
        else:
            return self.tokens["class_name"]

    @docs("namespace")
    def namespace_docs(self):
        if ns := self.root.gir.namespaces.get(self.tokens["namespace"]):
            return ns.doc

    @docs("class_name")
    def class_docs(self):
        if self.gir_type:
            return self.gir_type.doc

    @property
    def as_string(self) -> str:
        if self.tokens["extern"]:
            return "$" + self.tokens["class_name"]
        elif self.tokens["namespace"]:
            return f"{self.tokens['namespace']}.{self.tokens['class_name']}"
        else:
            return self.tokens["class_name"]


class ClassName(TypeName):
    @validate("namespace", "class_name")
    def gir_class_exists(self):
        if (
            self.gir_type is not None
            and not isinstance(self.gir_type, ExternType)
            and not isinstance(self.gir_type, Class)
        ):
            if isinstance(self.gir_type, Interface):
                raise CompileError(
                    f"{self.gir_type.full_name} is an interface, not a class"
                )
            else:
                raise CompileError(f"{self.gir_type.full_name} is not a class")


class ConcreteClassName(ClassName):
    @validate("namespace", "class_name")
    def not_abstract(self):
        if isinstance(self.gir_type, Class) and self.gir_type.abstract:
            raise CompileError(
                f"{self.gir_type.full_name} can't be instantiated because it's abstract",
                hints=[f"did you mean to use a subclass of {self.gir_type.full_name}?"],
            )


class TemplateClassName(ClassName):
    """Handles the special case of a template type. The old syntax uses an identifier,
    which is ambiguous with the new syntax. So this class displays an appropriate
    upgrade warning instead of a class not found error."""

    @property
    def is_legacy(self):
        return (
            self.tokens["extern"] is None
            and self.tokens["namespace"] is None
            and self.root.gir.get_type(self.tokens["class_name"], "Gtk") is None
        )

    @property
    def gir_type(self) -> gir.GirType:
        if self.is_legacy:
            return gir.ExternType(self.tokens["class_name"])
        else:
            return super().gir_type

    @validate("class_name")
    def type_exists(self):
        if self.is_legacy:
            if type := self.root.gir.get_type_by_cname(self.tokens["class_name"]):
                replacement = type.full_name
            else:
                replacement = "$" + self.tokens["class_name"]

            raise UpgradeWarning(
                "Use type syntax here (introduced in blueprint 0.8.0)",
                actions=[CodeAction("Use type syntax", replace_with=replacement)],
            )

        if not self.tokens["extern"] and self.gir_ns is not None:
            self.root.gir.validate_type(
                self.tokens["class_name"], self.tokens["namespace"]
            )
