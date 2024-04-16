# gobject_property.py
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


from .binding import Binding
from .common import *
from .contexts import ValueTypeCtx
from .gtkbuilder_template import Template
from .values import ObjectValue, Value


class Property(AstNode):
    grammar = Statement(UseIdent("name"), ":", AnyOf(Binding, ObjectValue, Value))

    @property
    def name(self) -> str:
        return self.tokens["name"]

    @property
    def value(self) -> T.Union[Binding, ObjectValue, Value]:
        return self.children[0]

    @property
    def gir_class(self):
        return self.parent.parent.gir_class

    @property
    def gir_property(self):
        if self.gir_class is not None and not isinstance(self.gir_class, ExternType):
            return self.gir_class.properties.get(self.tokens["name"])

    @validate()
    def binding_valid(self):
        if (
            isinstance(self.value, Binding)
            and self.gir_property is not None
            and self.gir_property.construct_only
        ):
            raise CompileError(
                f"{self.gir_property.full_name} can't be bound because it is construct-only",
                hints=["construct-only properties may only be set to a static value"],
            )

    @context(ValueTypeCtx)
    def value_type(self) -> ValueTypeCtx:
        if self.gir_property is not None:
            type = self.gir_property.type
        else:
            type = None

        return ValueTypeCtx(type)

    @validate("name")
    def property_exists(self):
        if self.gir_class is None or self.gir_class.incomplete:
            # Objects that we have no gir data on should not be validated
            # This happens for classes defined by the app itself
            return

        if self.gir_property is None:
            raise CompileError(
                f"Class {self.gir_class.full_name} does not have a property called {self.tokens['name']}",
                did_you_mean=(self.tokens["name"], self.gir_class.properties.keys()),
            )

    @validate("name")
    def property_writable(self):
        if self.gir_property is not None and not self.gir_property.writable:
            raise CompileError(f"{self.gir_property.full_name} is not writable")

    @validate("name")
    def unique_in_parent(self):
        self.validate_unique_in_parent(
            f"Duplicate property '{self.tokens['name']}'",
            check=lambda child: child.tokens["name"] == self.tokens["name"],
        )

    @docs("name")
    def property_docs(self):
        if self.gir_property is not None:
            return self.gir_property.doc
