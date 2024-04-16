# gtkbuilder_child.py
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


from functools import cached_property

from .common import *
from .gobject_object import Object
from .response_id import ExtResponse

ALLOWED_PARENTS: T.List[T.Tuple[str, str]] = [
    ("Gtk", "Buildable"),
    ("Gio", "ListStore"),
]


class ChildInternal(AstNode):
    grammar = ["internal-child", UseIdent("internal_child")]

    @property
    def internal_child(self) -> str:
        return self.tokens["internal_child"]


class ChildType(AstNode):
    grammar = UseIdent("child_type").expected("a child type")

    @property
    def child_type(self) -> str:
        return self.tokens["child_type"]


class ChildExtension(AstNode):
    grammar = ExtResponse

    @property
    def child(self) -> ExtResponse:
        return self.children[0]


class ChildAnnotation(AstNode):
    grammar = ["[", AnyOf(ChildInternal, ChildExtension, ChildType), "]"]

    @property
    def child(self) -> T.Union[ChildInternal, ChildExtension, ChildType]:
        return self.children[0]


class Child(AstNode):
    grammar = [
        Optional(ChildAnnotation),
        Object,
    ]

    @property
    def annotation(self) -> T.Optional[ChildAnnotation]:
        annotations = self.children[ChildAnnotation]
        return annotations[0] if len(annotations) else None

    @property
    def object(self) -> Object:
        return self.children[Object][0]

    @validate()
    def parent_can_have_child(self):
        if gir_class := self.parent.gir_class:
            for namespace, name in ALLOWED_PARENTS:
                parent_type = self.root.gir.get_type(name, namespace)
                if gir_class.assignable_to(parent_type):
                    break
            else:
                hints = [
                    "only Gio.ListStore or Gtk.Buildable implementors can have children"
                ]
                if "child" in gir_class.properties:
                    hints.append(
                        "did you mean to assign this object to the 'child' property?"
                    )
                raise CompileError(
                    f"{gir_class.full_name} doesn't have children",
                    hints=hints,
                )

    @cached_property
    def response_id(self) -> T.Optional[ExtResponse]:
        """Get action widget's response ID.

        If child is not action widget, returns `None`.
        """
        if (
            self.annotation is not None
            and isinstance(self.annotation.child, ChildExtension)
            and isinstance(self.annotation.child.child, ExtResponse)
        ):
            return self.annotation.child.child
        else:
            return None

    @validate()
    def internal_child_unique(self):
        if self.annotation is not None:
            if isinstance(self.annotation.child, ChildInternal):
                internal_child = self.annotation.child.internal_child
                self.validate_unique_in_parent(
                    f"Duplicate internal child '{internal_child}'",
                    lambda x: (
                        x.annotation
                        and isinstance(x.annotation.child, ChildInternal)
                        and x.annotation.child.internal_child == internal_child
                    ),
                )


@decompiler("child")
def decompile_child(ctx, gir, type=None, internal_child=None):
    if type is not None:
        ctx.print(f"[{type}]")
    elif internal_child is not None:
        ctx.print(f"[internal-child {internal_child}]")
    return gir
