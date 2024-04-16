# gtk_menus.py
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

import typing as T

from blueprintcompiler.language.values import StringValue

from .common import *
from .contexts import ValueTypeCtx
from .gobject_object import RESERVED_IDS


class Menu(AstNode):
    @property
    def gir_class(self):
        return self.root.gir.namespaces["Gtk"].lookup_type("Gio.Menu")

    @property
    def id(self) -> str:
        return self.tokens["id"]

    @property
    def signature(self) -> str:
        if self.id:
            return f"Gio.Menu {self.id}"
        else:
            return "Gio.Menu"

    @property
    def tag(self) -> str:
        return self.tokens["tag"]

    @property
    def items(self) -> T.List[T.Union["Menu", "MenuAttribute"]]:
        return self.children

    @validate("menu")
    def has_id(self):
        if self.tokens["tag"] == "menu" and self.tokens["id"] is None:
            raise CompileError("Menu requires an ID")

    @validate("id")
    def object_id_not_reserved(self):
        if self.id in RESERVED_IDS:
            raise CompileWarning(f"{self.id} may be a confusing object ID")


class MenuAttribute(AstNode):
    tag_name = "attribute"

    @property
    def name(self) -> str:
        return self.tokens["name"]

    @property
    def value(self) -> StringValue:
        return self.children[StringValue][0]

    @context(ValueTypeCtx)
    def value_type(self) -> ValueTypeCtx:
        return ValueTypeCtx(None)

    @validate("name")
    def unique(self):
        self.validate_unique_in_parent(
            f"Duplicate attribute '{self.name}'", lambda x: x.name == self.name
        )


menu_child = AnyOf()

menu_attribute = Group(
    MenuAttribute,
    [
        UseIdent("name"),
        ":",
        Err(StringValue, "Expected string or translated string"),
        Match(";").expected(),
    ],
)

menu_section = Group(
    Menu,
    [
        "section",
        UseLiteral("tag", "section"),
        Optional(UseIdent("id")),
        Match("{").expected(),
        Until(AnyOf(menu_child, menu_attribute), "}"),
    ],
)

menu_submenu = Group(
    Menu,
    [
        "submenu",
        UseLiteral("tag", "submenu"),
        Optional(UseIdent("id")),
        Match("{").expected(),
        Until(AnyOf(menu_child, menu_attribute), "}"),
    ],
)

menu_item = Group(
    Menu,
    [
        "item",
        UseLiteral("tag", "item"),
        Match("{").expected(),
        Until(menu_attribute, "}"),
    ],
)

menu_item_shorthand = Group(
    Menu,
    [
        "item",
        UseLiteral("tag", "item"),
        "(",
        Group(
            MenuAttribute,
            [UseLiteral("name", "label"), StringValue],
        ),
        Optional(
            [
                ",",
                Optional(
                    [
                        Group(
                            MenuAttribute,
                            [UseLiteral("name", "action"), StringValue],
                        ),
                        Optional(
                            [
                                ",",
                                Group(
                                    MenuAttribute,
                                    [UseLiteral("name", "icon"), StringValue],
                                ),
                            ]
                        ),
                    ]
                ),
            ]
        ),
        Match(")").expected(),
    ],
)

menu_child.children = [
    menu_section,
    menu_submenu,
    menu_item_shorthand,
    menu_item,
]

menu: Group = Group(
    Menu,
    [
        Keyword("menu"),
        UseLiteral("tag", "menu"),
        Optional(UseIdent("id")),
        [
            Match("{"),
            Until(
                AnyOf(
                    menu_child,
                    Fail(
                        menu_attribute,
                        "Attributes are not permitted at the top level of a menu",
                    ),
                ),
                "}",
            ),
        ],
    ],
)

from .ui import UI


@completer(
    applies_in=[UI],
    matches=new_statement_patterns,
)
def menu_completer(ast_node, match_variables):
    yield Completion("menu", CompletionItemKind.Snippet, snippet="menu {\n  $0\n}")


@completer(
    applies_in=[Menu],
    matches=new_statement_patterns,
)
def menu_content_completer(ast_node, match_variables):
    yield Completion(
        "submenu", CompletionItemKind.Snippet, snippet="submenu {\n  $0\n}"
    )
    yield Completion(
        "section", CompletionItemKind.Snippet, snippet="section {\n  $0\n}"
    )
    yield Completion("item", CompletionItemKind.Snippet, snippet="item {\n  $0\n}")
    yield Completion(
        "item (shorthand)",
        CompletionItemKind.Snippet,
        snippet='item (_("${1:Label}"), "${2:action-name}", "${3:icon-name}")',
    )

    yield Completion("label", CompletionItemKind.Snippet, snippet="label: $0;")
    yield Completion("action", CompletionItemKind.Snippet, snippet='action: "$0";')
    yield Completion("icon", CompletionItemKind.Snippet, snippet='icon: "$0";')


@decompiler("menu")
def decompile_menu(ctx, gir, id=None):
    if id:
        ctx.print(f"menu {id} {{")
    else:
        ctx.print("menu {")


@decompiler("submenu")
def decompile_submenu(ctx, gir, id=None):
    if id:
        ctx.print(f"submenu {id} {{")
    else:
        ctx.print("submenu {")


@decompiler("item")
def decompile_item(ctx, gir, id=None):
    if id:
        ctx.print(f"item {id} {{")
    else:
        ctx.print("item {")


@decompiler("section")
def decompile_section(ctx, gir, id=None):
    if id:
        ctx.print(f"section {id} {{")
    else:
        ctx.print("section {")
