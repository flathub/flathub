# gobject_object.py
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
from functools import cached_property

from .common import *
from .response_id import ExtResponse
from .types import ClassName, ConcreteClassName

RESERVED_IDS = {"this", "self", "template", "true", "false", "null", "none"}


class ObjectContent(AstNode):
    grammar = ["{", Until(OBJECT_CONTENT_HOOKS, "}")]

    @property
    def gir_class(self):
        return self.parent.gir_class


class Object(AstNode):
    grammar: T.Any = [
        ConcreteClassName,
        Optional(UseIdent("id")),
        ObjectContent,
    ]

    @property
    def id(self) -> str:
        return self.tokens["id"]

    @property
    def class_name(self) -> ClassName:
        return self.children[ClassName][0]

    @property
    def content(self) -> ObjectContent:
        return self.children[ObjectContent][0]

    @property
    def signature(self) -> str:
        if self.id:
            return f"{self.class_name.gir_type.full_name} {self.id}"
        else:
            return f"{self.class_name.gir_type.full_name}"

    @property
    def gir_class(self) -> GirType:
        if self.class_name is None:
            raise CompilerBugError()
        return self.class_name.gir_type

    @cached_property
    def action_widgets(self) -> T.List[ExtResponse]:
        """Get list of widget's action widgets.

        Empty if object doesn't have action widgets.
        """
        from .gtkbuilder_child import Child

        return [
            child.response_id
            for child in self.content.children[Child]
            if child.response_id
        ]

    @validate("id")
    def object_id_not_reserved(self):
        from .gtkbuilder_template import Template

        if not isinstance(self, Template) and self.id in RESERVED_IDS:
            raise CompileWarning(f"{self.id} may be a confusing object ID")


def validate_parent_type(node, ns: str, name: str, err_msg: str):
    parent = node.root.gir.get_type(name, ns)
    container_type = node.parent_by_type(Object).gir_class
    if container_type and not container_type.assignable_to(parent):
        raise CompileError(
            f"{container_type.full_name} is not a {parent.full_name}, so it doesn't have {err_msg}"
        )


@decompiler("object")
def decompile_object(ctx, gir, klass, id=None):
    gir_class = ctx.type_by_cname(klass)
    klass_name = (
        decompile.full_name(gir_class) if gir_class is not None else "$" + klass
    )
    if id is None:
        ctx.print(f"{klass_name} {{")
    else:
        ctx.print(f"{klass_name} {id} {{")
    return gir_class
