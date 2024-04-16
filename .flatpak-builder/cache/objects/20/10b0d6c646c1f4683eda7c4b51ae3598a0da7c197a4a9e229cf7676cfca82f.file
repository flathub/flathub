# -*- coding: utf-8 -*-

import os
from io import TextIOWrapper

__author__ = "Arijit Basu"
__email__ = "sayanarijit@gmail.com"
__homepage__ = "https://github.com/sayanarijit/expandvars"
__description__ = "Expand system variables Unix style"
__version__ = "v0.12.0"
__license__ = "MIT"
__all__ = [
    "BadSubstitution",
    "ExpandvarsException",
    "MissingClosingBrace",
    "MissingExcapedChar",
    "NegativeSubStringExpression",
    "OperandExpected",
    "ParameterNullOrNotSet",
    "UnboundVariable",
    "expand",
    "expandvars",
]


ESCAPE_CHAR = "\\"

# Set EXPANDVARS_RECOVER_NULL="foo" if you want variables with
# `${VAR:?}` syntax to fallback to "foo" if it's not defined.
# Also works works with nounset=True.
#
# This helps with certain use cases where you need to temporarily
# disable strict parsing of critical env vars. e.g. in testing
# environment.
#
# See tests/test_recover_null.py for examples.
#
# WARNING: Try to avoid `export EXPANDVARS_RECOVER_NULL` as it
# will permanently disable strict parsing until you log out.
RECOVER_NULL = os.environ.get("EXPANDVARS_RECOVER_NULL", None)


class ExpandvarsException(Exception):
    """The base exception for all the handleable exceptions."""

    pass


class MissingClosingBrace(ExpandvarsException, SyntaxError):
    def __init__(self, param):
        super().__init__("{0}: missing '}}'".format(param))


class MissingExcapedChar(ExpandvarsException, SyntaxError):
    def __init__(self, param):
        super().__init__("{0}: missing escaped character".format(param))


class OperandExpected(ExpandvarsException, SyntaxError):
    def __init__(self, param, operand):
        super().__init__(
            "{0}: operand expected (error token is {1})".format(param, repr(operand))
        )


class NegativeSubStringExpression(ExpandvarsException, IndexError):
    def __init__(self, param, expr):
        super().__init__("{0}: {1}: substring expression < 0".format(param, expr))


class BadSubstitution(ExpandvarsException, SyntaxError):
    def __init__(self, param):
        super().__init__("{0}: bad substitution".format(param))


class ParameterNullOrNotSet(ExpandvarsException, KeyError):
    def __init__(self, param, msg=None):
        if msg is None:
            msg = "parameter null or not set"
        super().__init__("{0}: {1}".format(param, msg))


class UnboundVariable(ExpandvarsException, KeyError):
    def __init__(self, param):
        super().__init__("{0}: unbound variable".format(param))


def _valid_char(char):
    return char.isalnum() or char == "_"


def _isint(val):
    try:
        int(val)
        return True
    except ValueError:
        return False


def getenv(var, nounset, indirect, environ, default=None):
    """Get value from environment variable.

    When nounset is True, it behaves like bash's "set -o nounset" or "set -u"
    and raises UnboundVariable exception.

    When indirect is True, it will use the value of the resolved variable as
    the name of the final variable.
    """

    val = environ.get(var)
    if val is not None and indirect:
        val = environ.get(val)

    if val:
        return val

    if default is not None:
        return default

    if nounset:
        if RECOVER_NULL is not None:
            return RECOVER_NULL
        raise UnboundVariable(var)
    return ""


def escape(vars_, nounset, environ, var_symbol):
    """Escape the first character."""
    if len(vars_) == 0:
        raise MissingExcapedChar(vars_)

    if len(vars_) == 1:
        return vars_[0]

    if vars_[0] == var_symbol:
        return vars_[0] + expand(vars_[1:], environ=environ, var_symbol=var_symbol)

    if vars_[0] == ESCAPE_CHAR:
        if vars_[1] == var_symbol:
            return ESCAPE_CHAR + expand(
                vars_[1:], nounset=nounset, environ=environ, var_symbol=var_symbol
            )
        if vars_[1] == ESCAPE_CHAR:
            return ESCAPE_CHAR + escape(
                vars_[2:], nounset=nounset, environ=environ, var_symbol=var_symbol
            )

    return (
        ESCAPE_CHAR
        + vars_[0]
        + expand(vars_[1:], nounset=nounset, environ=environ, var_symbol=var_symbol)
    )


def expand_var(vars_, nounset, environ, var_symbol):
    """Expand a single variable."""

    if len(vars_) == 0:
        return var_symbol

    if vars_[0] == ESCAPE_CHAR:
        return var_symbol + escape(
            vars_[1:], nounset=nounset, environ=environ, var_symbol=var_symbol
        )

    if vars_[0] == var_symbol:
        return str(os.getpid()) + expand(
            vars_[1:], nounset=nounset, environ=environ, var_symbol=var_symbol
        )

    if vars_[0] == "{":
        return expand_modifier_var(
            vars_[1:], nounset=nounset, environ=environ, var_symbol=var_symbol
        )

    buff = []
    for c in vars_:
        if _valid_char(c):
            buff.append(c)
        else:
            n = len(buff)
            return getenv(
                "".join(buff), nounset=nounset, indirect=False, environ=environ
            ) + expand(
                vars_[n:], nounset=nounset, environ=environ, var_symbol=var_symbol
            )
    return getenv("".join(buff), nounset=nounset, indirect=False, environ=environ)


def expand_modifier_var(vars_, nounset, environ, var_symbol):
    """Expand variables with modifier."""

    if len(vars_) <= 1:
        raise BadSubstitution(vars_)

    if vars_[0] == "!":
        indirect = True
        vars_ = vars_[1:]
    else:
        indirect = False

    buff = []
    for c in vars_:
        if _valid_char(c):
            buff.append(c)
        elif c == "}":
            n = len(buff) + 1
            return getenv(
                "".join(buff), nounset=nounset, indirect=indirect, environ=environ
            ) + expand(
                vars_[n:], nounset=nounset, environ=environ, var_symbol=var_symbol
            )
        else:
            n = len(buff)
            if c == ":":
                n += 1
            return expand_advanced(
                "".join(buff),
                vars_[n:],
                nounset=nounset,
                indirect=indirect,
                environ=environ,
                var_symbol=var_symbol,
            )

    raise MissingClosingBrace("".join(buff))


def expand_advanced(var, vars_, nounset, indirect, environ, var_symbol):
    """Expand substitution."""

    if len(vars_) == 0:
        raise MissingClosingBrace(var)

    modifier = []
    depth = 1
    for c in vars_:
        if c == "{":
            depth += 1
            modifier.append(c)
        elif c == "}":
            depth -= 1
            if depth == 0:
                break
            else:
                modifier.append(c)
        else:
            modifier.append(c)

    if depth != 0:
        raise MissingClosingBrace(var)

    vars_ = vars_[len(modifier) + 1 :]
    modifier = expand(
        "".join(modifier), nounset=nounset, environ=environ, var_symbol=var_symbol
    )

    if not modifier:
        raise BadSubstitution(var)

    if modifier[0] == "-":
        return expand_default(
            var,
            modifier=modifier[1:],
            set_=False,
            nounset=nounset,
            indirect=indirect,
            environ=environ,
        ) + expand(vars_, nounset=nounset, environ=environ, var_symbol=var_symbol)

    if modifier[0] == "=":
        return expand_default(
            var,
            modifier=modifier[1:],
            set_=True,
            nounset=nounset,
            indirect=indirect,
            environ=environ,
        ) + expand(vars_, nounset=nounset, environ=environ, var_symbol=var_symbol)

    if modifier[0] == "+":
        return expand_substitute(
            var,
            modifier=modifier[1:],
            environ=environ,
        ) + expand(vars_, nounset=nounset, environ=environ, var_symbol=var_symbol)

    if modifier[0] == "?":
        return expand_strict(
            var,
            modifier=modifier[1:],
            environ=environ,
        ) + expand(vars_, nounset=nounset, environ=environ, var_symbol=var_symbol)

    return expand_offset(
        var,
        modifier=modifier,
        nounset=nounset,
        environ=environ,
    ) + expand(vars_, nounset=nounset, environ=environ, var_symbol=var_symbol)


def expand_strict(var, modifier, environ):
    """Expand variable that must be defined."""

    val = environ.get(var, "")
    if val:
        return val
    if RECOVER_NULL is not None:
        return RECOVER_NULL
    raise ParameterNullOrNotSet(var, modifier if modifier else None)


def expand_offset(var, modifier, nounset, environ):
    """Expand variable with offset."""

    buff = []
    for c in modifier:
        if c == ":":
            n = len(buff) + 1
            offset_str = "".join(buff)
            if not offset_str or not _isint(offset_str):
                offset = 0
            else:
                offset = int(offset_str)

            return expand_length(
                var,
                modifier=modifier[n:],
                offset=offset,
                nounset=nounset,
                environ=environ,
            )

        buff.append(c)

    n = len(buff) + 1
    offset_str = "".join(buff).strip()
    if not offset_str or not _isint(offset_str):
        offset = 0
    else:
        offset = int(offset_str)
    return getenv(var, nounset=nounset, indirect=False, environ=environ)[offset:]


def expand_length(var, modifier, offset, nounset, environ):
    """Expand variable with offset and length."""

    length_str = modifier.strip()
    if not length_str:
        length = None
    elif not _isint(length_str):
        if not all(_valid_char(c) for c in length_str):
            raise OperandExpected(var, length_str)
        else:
            length = None
    else:
        length = int(length_str)
        if length < 0:
            raise NegativeSubStringExpression(var, length_str)

    if length is None:
        width = 0
    else:
        width = offset + length

    return getenv(var, nounset=nounset, indirect=False, environ=environ)[offset:width]


def expand_substitute(var, modifier, environ):
    """Expand or return substitute."""

    if environ.get(var):
        return modifier
    return ""


def expand_default(var, modifier, set_, nounset, indirect, environ):
    """Expand var or return default."""

    if set_ and not environ.get(var):
        environ.update({var: modifier})
    return getenv(
        var,
        nounset=nounset,
        indirect=indirect,
        default=modifier,
        environ=environ,
    )


def expand(vars_, nounset=False, environ=os.environ, var_symbol="$"):
    """Expand variables Unix style.

    Params:
        vars_ (str):  Variables to expand.
        nounset (bool): If True, enables strict parsing (similar to set -u / set -o nounset in bash).
        environ (Mapping): Elements to consider during variable expansion. Defaults to os.environ
        var_symbol (str): Character used to identify a variable. Defaults to $

    Returns:
        str: Expanded values.

    Example usage: ::

        from expandvars import expand

        print(expand("%PATH:$HOME/bin:%{SOME_UNDEFINED_PATH:-/default/path}", environ={"PATH": "/example"}, var_symbol="%"))
        # /example:$HOME/bin:/default/path

        # Or
        with open(somefile) as f:
            print(expand(f))
    """
    if isinstance(vars_, TextIOWrapper):
        # This is a file. Read it.
        vars_ = vars_.read().strip()

    if len(vars_) == 0:
        return ""

    buff = []

    try:
        for c in vars_:
            if c == var_symbol:
                n = len(buff) + 1
                return "".join(buff) + expand_var(
                    vars_[n:], nounset=nounset, environ=environ, var_symbol=var_symbol
                )

            if c == ESCAPE_CHAR:
                n = len(buff) + 1
                return "".join(buff) + escape(
                    vars_[n:], nounset=nounset, environ=environ, var_symbol=var_symbol
                )

            buff.append(c)
        return "".join(buff)
    except MissingExcapedChar:
        raise MissingExcapedChar(vars_)
    except MissingClosingBrace:
        raise MissingClosingBrace(vars_)
    except BadSubstitution:
        raise BadSubstitution(vars_)


def expandvars(vars_, nounset=False):
    """Expand system variables Unix style.

    Params:
        vars_ (str): System variables to expand.
        nounset (bool): If True, enables strict parsing (similar to set -u / set -o nounset in bash).

    Returns:
        str: Expanded values.

    Example usage: ::

        from expandvars import expandvars

        print(expandvars("$PATH:$HOME/bin:${SOME_UNDEFINED_PATH:-/default/path}"))
        # /bin:/sbin:/usr/bin:/usr/sbin:/home/you/bin:/default/path

        # Or
        with open(somefile) as f:
            print(expandvars(f))
    """
    return expand(vars_, nounset=nounset)
