# parser.py
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


from .errors import MultipleErrors, PrintableError
from .language import OBJECT_CONTENT_HOOKS, UI, Template
from .parse_tree import *
from .tokenizer import TokenType


def parse(
    tokens: T.List[Token],
) -> T.Tuple[T.Optional[UI], T.Optional[MultipleErrors], T.List[PrintableError]]:
    """Parses a list of tokens into an abstract syntax tree."""

    try:
        ctx = ParseContext(tokens)
        AnyOf(UI).parse(ctx)
        ast_node = ctx.last_group.to_ast() if ctx.last_group else None

        errors = [*ctx.errors, *ast_node.errors]
        warnings = [*ctx.warnings, *ast_node.warnings]

        return (ast_node, MultipleErrors(errors) if len(errors) else None, warnings)
    except MultipleErrors as e:
        return (None, e, [])
    except CompileError as e:
        return (None, MultipleErrors([e]), [])
