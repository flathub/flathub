# parse_tree.py
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

""" Utilities for parsing an AST from a token stream. """

import typing as T
from collections import defaultdict
from enum import Enum

from .ast_utils import AstNode
from .errors import (
    CompileError,
    CompilerBugError,
    CompileWarning,
    UnexpectedTokenError,
    assert_true,
)
from .tokenizer import Token, TokenType

SKIP_TOKENS = [TokenType.COMMENT, TokenType.WHITESPACE]


class ParseResult(Enum):
    """Represents the result of parsing. The extra EMPTY result is necessary
    to avoid freezing the parser: imagine a ZeroOrMore node containing a node
    that can match empty. It will repeatedly match empty and never advance
    the parser. So, ZeroOrMore stops when a failed *or empty* match is
    made."""

    SUCCESS = 0
    FAILURE = 1
    EMPTY = 2

    def matched(self):
        return self == ParseResult.SUCCESS

    def succeeded(self):
        return self != ParseResult.FAILURE

    def failed(self):
        return self == ParseResult.FAILURE


class ParseGroup:
    """A matching group. Match groups have an AST type, children grouped by
    type, and key=value pairs. At the end of parsing, the match groups will
    be converted to AST nodes by passing the children and key=value pairs to
    the AST node constructor."""

    def __init__(self, ast_type: T.Type[AstNode], start: int):
        self.ast_type = ast_type
        self.children: T.List[ParseGroup] = []
        self.keys: T.Dict[str, T.Any] = {}
        self.tokens: T.Dict[str, T.Optional[Token]] = {}
        self.start = start
        self.end: T.Optional[int] = None
        self.incomplete = False

    def add_child(self, child: "ParseGroup"):
        self.children.append(child)

    def set_val(self, key: str, val: T.Any, token: T.Optional[Token]):
        assert_true(key not in self.keys)

        self.keys[key] = val
        self.tokens[key] = token

    def to_ast(self):
        """Creates an AST node from the match group."""
        children = [child.to_ast() for child in self.children]

        try:
            return self.ast_type(self, children, self.keys, incomplete=self.incomplete)
        except TypeError as e:
            raise CompilerBugError(
                f"Failed to construct ast.{self.ast_type.__name__} from ParseGroup. See the previous stacktrace."
            )

    def __str__(self):
        result = str(self.ast_type.__name__)
        result += "".join([f"\n{key}: {val}" for key, val in self.keys.items()]) + "\n"
        result += "\n".join(
            [str(child) for children in self.children.values() for child in children]
        )
        return result.replace("\n", "\n  ")


class ParseContext:
    """Contains the state of the parser."""

    def __init__(self, tokens: T.List[Token], index=0):
        self.tokens = tokens

        self.binding_power = 0
        self.index = index
        self.start = index
        self.group: T.Optional[ParseGroup] = None
        self.group_keys: T.Dict[str, T.Tuple[T.Any, T.Optional[Token]]] = {}
        self.group_children: T.List[ParseGroup] = []
        self.last_group: T.Optional[ParseGroup] = None
        self.group_incomplete = False

        self.errors: T.List[CompileError] = []
        self.warnings: T.List[CompileWarning] = []

    def create_child(self) -> "ParseContext":
        """Creates a new ParseContext at this context's position. The new
        context will be used to parse one node. If parsing is successful, the
        new context will be applied to "self". If parsing fails, the new
        context will be discarded."""
        ctx = ParseContext(self.tokens, self.index)
        ctx.errors = self.errors
        ctx.warnings = self.warnings
        ctx.binding_power = self.binding_power
        return ctx

    def apply_child(self, other: "ParseContext"):
        """Applies a child context to this context."""

        if other.group is not None:
            # If the other context had a match group, collect all the matched
            # values into it and then add it to our own match group.
            for key, (val, token) in other.group_keys.items():
                other.group.set_val(key, val, token)
            for child in other.group_children:
                other.group.add_child(child)
            other.group.end = other.tokens[other.index - 1].end
            other.group.incomplete = other.group_incomplete
            self.group_children.append(other.group)
        else:
            # If the other context had no match group of its own, collect all
            # its matched values
            self.group_keys = {**self.group_keys, **other.group_keys}
            self.group_children += other.group_children
            self.group_incomplete |= other.group_incomplete

        self.index = other.index
        # Propagate the last parsed group down the stack so it can be easily
        # retrieved at the end of the process
        if other.group:
            self.last_group = other.group
        elif other.last_group:
            self.last_group = other.last_group

    def start_group(self, ast_type: T.Type[AstNode]):
        """Sets this context to have its own match group."""
        assert_true(self.group is None)
        self.group = ParseGroup(ast_type, self.tokens[self.index].start)

    def set_group_val(self, key: str, value: T.Any, token: T.Optional[Token]):
        """Sets a matched key=value pair on the current match group."""
        assert_true(key not in self.group_keys)
        self.group_keys[key] = (value, token)

    def set_group_incomplete(self):
        """Marks the current match group as incomplete (it could not be fully
        parsed, but the parser recovered)."""
        self.group_incomplete = True

    def skip(self):
        """Skips whitespace and comments."""
        while (
            self.index < len(self.tokens)
            and self.tokens[self.index].type in SKIP_TOKENS
        ):
            self.index += 1

    def next_token(self) -> Token:
        """Advances the token iterator and returns the next token."""
        self.skip()
        token = self.tokens[self.index]
        self.index += 1
        return token

    def peek_token(self) -> Token:
        """Returns the next token without advancing the iterator."""
        self.skip()
        token = self.tokens[self.index]
        return token

    def skip_unexpected_token(self):
        """Skips a token and logs an "unexpected token" error."""

        self.skip()
        start = self.tokens[self.index].start
        self.next_token()
        self.skip()
        end = self.tokens[self.index - 1].end

        if (
            len(self.errors)
            and isinstance((err := self.errors[-1]), UnexpectedTokenError)
            and err.end == start
        ):
            err.end = end
        else:
            self.errors.append(UnexpectedTokenError(start, end))

    def is_eof(self) -> bool:
        return self.index >= len(self.tokens) or self.peek_token().type == TokenType.EOF


class ParseNode:
    """Base class for the nodes in the parser tree."""

    def parse(self, ctx: ParseContext) -> ParseResult:
        """Attempts to match the ParseNode at the context's current location."""
        start_idx = ctx.index
        inner_ctx = ctx.create_child()

        if self._parse(inner_ctx):
            ctx.apply_child(inner_ctx)
            if ctx.index == start_idx:
                return ParseResult.EMPTY
            else:
                return ParseResult.SUCCESS
        else:
            return ParseResult.FAILURE

    def _parse(self, ctx: ParseContext) -> bool:
        raise NotImplementedError()

    def err(self, message: str) -> "Err":
        """Causes this ParseNode to raise an exception if it fails to parse.
        This prevents the parser from backtracking, so you should understand
        what it does and how the parser works before using it."""
        return Err(self, message)

    def expected(self, expect: str) -> "Err":
        """Convenience method for err()."""
        return self.err("Expected " + expect)

    def warn(self, message) -> "Warning":
        """Causes this ParseNode to emit a warning if it parses successfully."""
        return Warning(self, message)


class Err(ParseNode):
    """ParseNode that emits a compile error if it fails to parse."""

    def __init__(self, child, message: str):
        self.child = to_parse_node(child)
        self.message = message

    def _parse(self, ctx: ParseContext):
        if self.child.parse(ctx).failed():
            start_idx = ctx.start
            while ctx.tokens[start_idx].type in SKIP_TOKENS:
                start_idx += 1

            start_token = ctx.tokens[start_idx]
            end_token = ctx.tokens[ctx.index]
            raise CompileError(self.message, start_token.start, end_token.end)
        return True


class Warning(ParseNode):
    """ParseNode that emits a compile warning if it parses successfully."""

    def __init__(self, child, message: str):
        self.child = to_parse_node(child)
        self.message = message

    def _parse(self, ctx: ParseContext):
        ctx.skip()
        start_idx = ctx.index
        if self.child.parse(ctx).succeeded():
            start_token = ctx.tokens[start_idx]
            end_token = ctx.tokens[ctx.index]
            ctx.warnings.append(
                CompileWarning(self.message, start_token.start, end_token.end)
            )
            return True
        else:
            return False


class Fail(ParseNode):
    """ParseNode that emits a compile error if it parses successfully."""

    def __init__(self, child, message: str):
        self.child = to_parse_node(child)
        self.message = message

    def _parse(self, ctx: ParseContext):
        if self.child.parse(ctx).succeeded():
            start_idx = ctx.start
            while ctx.tokens[start_idx].type in SKIP_TOKENS:
                start_idx += 1

            start_token = ctx.tokens[start_idx]
            end_token = ctx.tokens[ctx.index]
            raise CompileError(self.message, start_token.start, end_token.end)
        return True


class Group(ParseNode):
    """ParseNode that creates a match group."""

    def __init__(self, ast_type: T.Type[AstNode], child):
        self.ast_type = ast_type
        self.child = to_parse_node(child)

    def _parse(self, ctx: ParseContext) -> bool:
        ctx.skip()
        ctx.start_group(self.ast_type)
        return self.child.parse(ctx).succeeded()


class Sequence(ParseNode):
    """ParseNode that attempts to match all of its children in sequence."""

    def __init__(self, *children):
        self.children = [to_parse_node(child) for child in children]

    def _parse(self, ctx) -> bool:
        for child in self.children:
            if child.parse(ctx).failed():
                return False
        return True


class Statement(ParseNode):
    """ParseNode that attempts to match all of its children in sequence. If any
    child raises an error, the error will be logged but parsing will continue."""

    def __init__(self, *children):
        self.children = [to_parse_node(child) for child in children]

    def _parse(self, ctx) -> bool:
        for child in self.children:
            try:
                if child.parse(ctx).failed():
                    return False
            except CompileError as e:
                ctx.errors.append(e)
                ctx.set_group_incomplete()
                return True

        token = ctx.peek_token()
        if str(token) != ";":
            ctx.errors.append(CompileError("Expected `;`", token.start, token.end))
        else:
            ctx.next_token()
        return True


class AnyOf(ParseNode):
    """ParseNode that attempts to match exactly one of its children. Child
    nodes are attempted in order."""

    def __init__(self, *children):
        self.children = children

    @property
    def children(self):
        return self._children

    @children.setter
    def children(self, children):
        self._children = [to_parse_node(child) for child in children]

    def _parse(self, ctx):
        for child in self.children:
            if child.parse(ctx).succeeded():
                return True
        return False


class Until(ParseNode):
    """ParseNode that repeats its child until a delimiting token is found. If
    the child does not match, one token is skipped and the match is attempted
    again."""

    def __init__(self, child, delimiter, between_delimiter=None):
        self.child = to_parse_node(child)
        self.delimiter = to_parse_node(delimiter)
        self.between_delimiter = (
            to_parse_node(between_delimiter) if between_delimiter is not None else None
        )

    def _parse(self, ctx: ParseContext):
        while not self.delimiter.parse(ctx).succeeded():
            if ctx.is_eof():
                return False

            try:
                if not self.child.parse(ctx).matched():
                    ctx.skip_unexpected_token()

                if (
                    self.between_delimiter is not None
                    and not self.between_delimiter.parse(ctx).succeeded()
                ):
                    if self.delimiter.parse(ctx).succeeded():
                        return True
                    else:
                        if ctx.is_eof():
                            return False
                        ctx.skip_unexpected_token()
            except CompileError as e:
                ctx.errors.append(e)
                ctx.next_token()

        return True


class ZeroOrMore(ParseNode):
    """ParseNode that matches its child any number of times (including zero
    times). It cannot fail to parse. If its child raises an exception, one token
    will be skipped and parsing will continue."""

    def __init__(self, child):
        self.child = to_parse_node(child)

    def _parse(self, ctx):
        while True:
            try:
                if not self.child.parse(ctx).matched():
                    return True
            except CompileError as e:
                ctx.errors.append(e)
                ctx.next_token()


class Delimited(ParseNode):
    """ParseNode that matches its first child any number of times (including zero
    times) with its second child in between and optionally at the end."""

    def __init__(self, child, delimiter):
        self.child = to_parse_node(child)
        self.delimiter = to_parse_node(delimiter)

    def _parse(self, ctx):
        while self.child.parse(ctx).matched() and self.delimiter.parse(ctx).matched():
            pass
        return True


class Optional(ParseNode):
    """ParseNode that matches its child zero or one times. It cannot fail to
    parse."""

    def __init__(self, child):
        self.child = to_parse_node(child)

    def _parse(self, ctx):
        self.child.parse(ctx)
        return True


class Eof(ParseNode):
    """ParseNode that matches an EOF token."""

    def _parse(self, ctx: ParseContext) -> bool:
        token = ctx.next_token()
        return token.type == TokenType.EOF


class Match(ParseNode):
    """ParseNode that matches the given literal token."""

    def __init__(self, op: str):
        self.op = op

    def _parse(self, ctx: ParseContext) -> bool:
        token = ctx.next_token()
        return str(token) == self.op

    def expected(self, expect: T.Optional[str] = None):
        """Convenience method for err()."""
        if expect is None:
            return self.err(f"Expected '{self.op}'")
        else:
            return self.err("Expected " + expect)


class UseIdent(ParseNode):
    """ParseNode that matches any identifier and sets it in a key=value pair on
    the containing match group."""

    def __init__(self, key: str):
        self.key = key

    def _parse(self, ctx: ParseContext):
        token = ctx.next_token()
        if token.type != TokenType.IDENT:
            return False

        ctx.set_group_val(self.key, str(token), token)
        return True


class UseNumber(ParseNode):
    """ParseNode that matches a number and sets it in a key=value pair on
    the containing match group."""

    def __init__(self, key: str):
        self.key = key

    def _parse(self, ctx: ParseContext):
        token = ctx.next_token()
        if token.type != TokenType.NUMBER:
            return False

        number = token.get_number()
        ctx.set_group_val(self.key, number, token)
        return True


class UseNumberText(ParseNode):
    """ParseNode that matches a number, but sets its *original text* it in a
    key=value pair on the containing match group."""

    def __init__(self, key: str):
        self.key = key

    def _parse(self, ctx: ParseContext):
        token = ctx.next_token()
        if token.type != TokenType.NUMBER:
            return False

        ctx.set_group_val(self.key, str(token), token)
        return True


class UseQuoted(ParseNode):
    """ParseNode that matches a quoted string and sets it in a key=value pair
    on the containing match group."""

    def __init__(self, key: str):
        self.key = key

    def _parse(self, ctx: ParseContext):
        token = ctx.next_token()
        if token.type != TokenType.QUOTED:
            return False

        string = (
            str(token)[1:-1]
            .replace("\\n", "\n")
            .replace('\\"', '"')
            .replace("\\\\", "\\")
            .replace("\\'", "'")
        )
        ctx.set_group_val(self.key, string, token)
        return True


class UseLiteral(ParseNode):
    """ParseNode that doesn't match anything, but rather sets a static key=value
    pair on the containing group. Useful for, e.g., property and signal flags:
    `Sequence(Keyword("swapped"), UseLiteral("swapped", True))`"""

    def __init__(self, key: str, literal: T.Any):
        self.key = key
        self.literal = literal

    def _parse(self, ctx: ParseContext):
        ctx.set_group_val(self.key, self.literal, None)
        return True


class UseExact(ParseNode):
    """Matches the given identifier and sets it as a named token."""

    def __init__(self, key: str, string: str):
        self.key = key
        self.string = string

    def _parse(self, ctx: ParseContext):
        token = ctx.next_token()
        ctx.set_group_val(self.key, self.string, token)
        return str(token) == self.string


class Keyword(ParseNode):
    """Matches the given identifier and sets it as a named token, with the name
    being the identifier itself."""

    def __init__(self, kw: str):
        self.kw = kw
        self.set_token = True

    def _parse(self, ctx: ParseContext):
        token = ctx.next_token()
        ctx.set_group_val(self.kw, True, token)
        return str(token) == self.kw


def to_parse_node(value) -> ParseNode:
    if isinstance(value, str):
        return Match(value)
    elif isinstance(value, list):
        return Sequence(*value)
    elif isinstance(value, type) and hasattr(value, "grammar"):
        return Group(value, getattr(value, "grammar"))
    elif isinstance(value, ParseNode):
        return value
    else:
        raise CompilerBugError()
