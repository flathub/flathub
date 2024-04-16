# completions_utils.py
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

from .lsp_utils import Completion
from .tokenizer import Token, TokenType

new_statement_patterns = [
    [(TokenType.PUNCTUATION, "{")],
    [(TokenType.PUNCTUATION, "}")],
    [(TokenType.PUNCTUATION, "]")],
    [(TokenType.PUNCTUATION, ";")],
]


def applies_to(*ast_types):
    """Decorator describing which AST nodes the completer should apply in."""

    def decorator(func):
        for c in ast_types:
            c.completers.append(func)
        return func

    return decorator


def completer(applies_in: T.List, matches: T.List = [], applies_in_subclass=None):
    def decorator(func):
        def inner(prev_tokens: T.List[Token], ast_node):
            # For completers that apply in ObjectContent nodes, we can further
            # check that the object is the right class
            if applies_in_subclass is not None:
                type = ast_node.root.gir.get_type(
                    applies_in_subclass[1], applies_in_subclass[0]
                )
                if ast_node.gir_class and not ast_node.gir_class.assignable_to(type):
                    return

            any_match = len(matches) == 0
            match_variables: T.List[str] = []

            for pattern in matches:
                match_variables = []

                if len(pattern) <= len(prev_tokens):
                    for i in range(0, len(pattern)):
                        type, value = pattern[i]
                        token = prev_tokens[i - len(pattern)]
                        if token.type != type or (
                            value is not None and str(token) != value
                        ):
                            break
                        if value is None:
                            match_variables.append(str(token))
                    else:
                        any_match = True
                        break

            if not any_match:
                return

            yield from func(ast_node, match_variables)

        for c in applies_in:
            c.completers.append(inner)
        return inner

    return decorator
