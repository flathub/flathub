# gtkbuilder_template.py
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

from blueprintcompiler.language.common import GirType

from ..gir import TemplateType
from .common import *
from .gobject_object import Object, ObjectContent
from .types import ClassName, TemplateClassName


class Template(Object):
    grammar = [
        UseExact("id", "template"),
        to_parse_node(TemplateClassName).expected("template type"),
        Optional(
            [
                Match(":"),
                to_parse_node(ClassName).expected("parent class"),
            ]
        ),
        ObjectContent,
    ]

    @property
    def id(self) -> str:
        return "template"

    @property
    def signature(self) -> str:
        if self.parent_type:
            return f"template {self.gir_class.full_name} : {self.parent_type.gir_type.full_name}"
        else:
            return f"template {self.gir_class.full_name}"

    @property
    def gir_class(self) -> GirType:
        if isinstance(self.class_name.gir_type, ExternType):
            if gir := self.parent_type:
                return TemplateType(self.class_name.gir_type.full_name, gir.gir_type)
        return self.class_name.gir_type

    @property
    def parent_type(self) -> T.Optional[ClassName]:
        if len(self.children[ClassName]) == 2:
            return self.children[ClassName][1]
        else:
            return None

    @validate()
    def parent_only_if_extern(self):
        if not isinstance(self.class_name.gir_type, ExternType):
            if self.parent_type is not None:
                raise CompileError(
                    "Parent type may only be specified if the template type is extern"
                )

    @validate("id")
    def unique_in_parent(self):
        self.validate_unique_in_parent(
            f"Only one template may be defined per file, but this file contains {len(self.parent.children[Template])}",
        )


@decompiler("template")
def decompile_template(ctx: DecompileCtx, gir, klass, parent=None):
    def class_name(cname: str) -> str:
        if gir := ctx.type_by_cname(cname):
            return decompile.full_name(gir)
        else:
            return "$" + cname

    ctx.print(f"template {class_name(klass)} : {class_name(parent)} {{")

    ctx.template_class = klass

    return ctx.type_by_cname(klass) or ctx.type_by_cname(parent)
