# completions.py
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

from . import gir, language
from .ast_utils import AstNode
from .completions_utils import *
from .language.types import ClassName
from .lsp_utils import Completion, CompletionItemKind
from .parser import SKIP_TOKENS
from .tokenizer import Token, TokenType

Pattern = T.List[T.Tuple[TokenType, T.Optional[str]]]


def _complete(
    ast_node: AstNode, tokens: T.List[Token], idx: int, token_idx: int
) -> T.Iterator[Completion]:
    for child in ast_node.children:
        if child.group.start <= idx and (
            idx < child.group.end or (idx == child.group.end and child.incomplete)
        ):
            yield from _complete(child, tokens, idx, token_idx)
            return

    prev_tokens: T.List[Token] = []

    # collect the 5 previous non-skipped tokens
    while len(prev_tokens) < 5 and token_idx >= 0:
        token = tokens[token_idx]
        if token.type not in SKIP_TOKENS:
            prev_tokens.insert(0, token)
        token_idx -= 1

    for completer in ast_node.completers:
        yield from completer(prev_tokens, ast_node)


def complete(
    ast_node: AstNode, tokens: T.List[Token], idx: int
) -> T.Iterator[Completion]:
    token_idx = 0
    # find the current token
    for i, token in enumerate(tokens):
        if token.start < idx <= token.end:
            token_idx = i

    # if the current token is an identifier or whitespace, move to the token before it
    while tokens[token_idx].type in [TokenType.IDENT, TokenType.WHITESPACE]:
        idx = tokens[token_idx].start
        token_idx -= 1

    yield from _complete(ast_node, tokens, idx, token_idx)


@completer([language.GtkDirective])
def using_gtk(ast_node, match_variables):
    yield Completion("using Gtk 4.0;", CompletionItemKind.Keyword)


@completer(
    applies_in=[language.UI, language.ObjectContent, language.Template],
    matches=new_statement_patterns,
)
def namespace(ast_node, match_variables):
    yield Completion("Gtk", CompletionItemKind.Module, text="Gtk.")
    for ns in ast_node.root.children[language.Import]:
        if ns.gir_namespace is not None:
            yield Completion(
                ns.gir_namespace.name,
                CompletionItemKind.Module,
                text=ns.gir_namespace.name + ".",
            )


@completer(
    applies_in=[language.UI, language.ObjectContent, language.Template],
    matches=[
        [(TokenType.IDENT, None), (TokenType.OP, "."), (TokenType.IDENT, None)],
        [(TokenType.IDENT, None), (TokenType.OP, ".")],
    ],
)
def object_completer(ast_node, match_variables):
    ns = ast_node.root.gir.namespaces.get(match_variables[0])
    if ns is not None:
        for c in ns.classes.values():
            yield Completion(c.name, CompletionItemKind.Class, docs=c.doc)


@completer(
    applies_in=[language.UI, language.ObjectContent, language.Template],
    matches=new_statement_patterns,
)
def gtk_object_completer(ast_node, match_variables):
    ns = ast_node.root.gir.namespaces.get("Gtk")
    if ns is not None:
        for c in ns.classes.values():
            yield Completion(c.name, CompletionItemKind.Class, docs=c.doc)


@completer(
    applies_in=[language.ObjectContent],
    matches=new_statement_patterns,
)
def property_completer(ast_node, match_variables):
    if ast_node.gir_class and not isinstance(ast_node.gir_class, gir.ExternType):
        for prop in ast_node.gir_class.properties:
            yield Completion(prop, CompletionItemKind.Property, snippet=f"{prop}: $0;")


@completer(
    applies_in=[language.Property, language.BaseAttribute],
    matches=[[(TokenType.IDENT, None), (TokenType.OP, ":")]],
)
def prop_value_completer(ast_node, match_variables):
    if isinstance(ast_node.value_type, gir.Enumeration):
        for name, member in ast_node.value_type.members.items():
            yield Completion(name, CompletionItemKind.EnumMember, docs=member.doc)

    elif isinstance(ast_node.value_type, gir.BoolType):
        yield Completion("true", CompletionItemKind.Constant)
        yield Completion("false", CompletionItemKind.Constant)


@completer(
    applies_in=[language.ObjectContent],
    matches=new_statement_patterns,
)
def signal_completer(ast_node, match_variables):
    if ast_node.gir_class and not isinstance(ast_node.gir_class, gir.ExternType):
        for signal in ast_node.gir_class.signals:
            if not isinstance(ast_node.parent, language.Object):
                name = "on"
            else:
                name = "on_" + (
                    ast_node.parent.children[ClassName][0].tokens["id"]
                    or ast_node.parent.children[ClassName][0]
                    .tokens["class_name"]
                    .lower()
                )
            yield Completion(
                signal,
                CompletionItemKind.Property,
                snippet=f"{signal} => ${{1:{name}_{signal.replace('-', '_')}}}()$0;",
            )


@completer(applies_in=[language.UI], matches=new_statement_patterns)
def template_completer(ast_node, match_variables):
    yield Completion(
        "template",
        CompletionItemKind.Snippet,
        snippet="template ${1:ClassName} : ${2:ParentClass} {\n  $0\n}",
    )
