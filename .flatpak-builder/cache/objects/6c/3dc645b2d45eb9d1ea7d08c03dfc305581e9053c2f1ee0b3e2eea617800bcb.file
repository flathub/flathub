# ui.py
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

from .. import gir
from .common import *
from .contexts import ScopeCtx
from .gobject_object import Object
from .gtk_menu import Menu, menu
from .gtkbuilder_template import Template
from .imports import GtkDirective, Import


class UI(AstNode):
    """The AST node for the entire file"""

    grammar = [
        GtkDirective,
        ZeroOrMore(Import),
        Until(
            AnyOf(
                Template,
                menu,
                Object,
            ),
            Eof(),
        ),
    ]

    @cached_property
    def gir(self) -> gir.GirContext:
        gir_ctx = gir.GirContext()
        self._gir_errors = []

        try:
            if gtk := self.children[GtkDirective][0].gir_namespace:
                gir_ctx.add_namespace(gtk)
        except CompileError as e:
            self._gir_errors.append(e)

        for i in self.children[Import]:
            try:
                if i.gir_namespace is not None:
                    gir_ctx.add_namespace(i.gir_namespace)
                else:
                    gir_ctx.not_found_namespaces.add(i.namespace)
            except CompileError as e:
                e.start = i.group.tokens["namespace"].start
                e.end = i.group.tokens["version"].end
                self._gir_errors.append(e)

        return gir_ctx

    @property
    def using(self) -> T.List[Import]:
        return self.children[Import]

    @property
    def gtk_decl(self) -> GtkDirective:
        return self.children[GtkDirective][0]

    @property
    def contents(self) -> T.List[T.Union[Object, Template, Menu]]:
        return [
            child
            for child in self.children
            if isinstance(child, Object)
            or isinstance(child, Template)
            or isinstance(child, Menu)
        ]

    @property
    def template(self) -> T.Optional[Template]:
        if len(self.children[Template]):
            return self.children[Template][0]
        else:
            return None

    def is_legacy_template(self, id: str) -> bool:
        return (
            id not in self.context[ScopeCtx].objects
            and self.template is not None
            and self.template.class_name.glib_type_name == id
        )

    @context(ScopeCtx)
    def scope_ctx(self) -> ScopeCtx:
        return ScopeCtx(node=self)

    @validate()
    def gir_errors(self):
        # make sure gir is loaded
        self.gir
        if len(self._gir_errors):
            raise MultipleErrors(self._gir_errors)

    @validate()
    def unique_ids(self):
        self.context[ScopeCtx].validate_unique_ids()
