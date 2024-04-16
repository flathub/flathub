# decompiler.py
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

import re
import typing as T
from dataclasses import dataclass
from enum import Enum

from .gir import *
from .utils import Colors
from .xml_reader import Element, parse, parse_string

__all__ = ["decompile"]


_DECOMPILERS: T.Dict = {}
_CLOSING = {
    "{": "}",
    "[": "]",
}
_NAMESPACES = [
    ("GLib", "2.0"),
    ("GObject", "2.0"),
    ("Gio", "2.0"),
    ("Adw", "1"),
]


class LineType(Enum):
    NONE = 1
    STMT = 2
    BLOCK_START = 3
    BLOCK_END = 4


class DecompileCtx:
    def __init__(self) -> None:
        self._result: str = ""
        self.gir = GirContext()
        self._indent: int = 0
        self._blocks_need_end: T.List[str] = []
        self._last_line_type: LineType = LineType.NONE
        self.template_class: T.Optional[str] = None

        self.gir.add_namespace(get_namespace("Gtk", "4.0"))

    @property
    def result(self) -> str:
        imports = "\n".join(
            [
                f"using {ns} {namespace.version};"
                for ns, namespace in self.gir.namespaces.items()
            ]
        )
        return imports + "\n" + self._result

    def type_by_cname(self, cname: str) -> T.Optional[GirType]:
        if type := self.gir.get_type_by_cname(cname):
            return type

        for ns, version in _NAMESPACES:
            try:
                namespace = get_namespace(ns, version)
                if type := namespace.get_type_by_cname(cname):
                    self.gir.add_namespace(namespace)
                    return type
            except:
                pass

        return None

    def start_block(self) -> None:
        self._blocks_need_end.append("")

    def end_block(self) -> None:
        if close := self._blocks_need_end.pop():
            self.print(close)

    def end_block_with(self, text: str) -> None:
        self._blocks_need_end[-1] = text

    def print(self, line: str, newline: bool = True) -> None:
        if line == "}" or line == "]":
            self._indent -= 1

        # Add blank lines between different types of lines, for neatness
        if newline:
            if line == "}" or line == "]":
                line_type = LineType.BLOCK_END
            elif line.endswith("{") or line.endswith("]"):
                line_type = LineType.BLOCK_START
            elif line.endswith(";"):
                line_type = LineType.STMT
            else:
                line_type = LineType.NONE
            if (
                line_type != self._last_line_type
                and self._last_line_type != LineType.BLOCK_START
                and line_type != LineType.BLOCK_END
            ):
                self._result += "\n"
            self._last_line_type = line_type

        self._result += ("  " * self._indent) + line
        if newline:
            self._result += "\n"

        if line.endswith("{") or line.endswith("["):
            if len(self._blocks_need_end):
                self._blocks_need_end[-1] = _CLOSING[line[-1]]
            self._indent += 1

    def print_attribute(self, name: str, value: str, type: GirType) -> None:
        def get_enum_name(value):
            for member in type.members.values():
                if (
                    member.nick == value
                    or member.c_ident == value
                    or str(member.value) == value
                ):
                    return member.name
            return value.replace("-", "_")

        if type is None:
            self.print(f'{name}: "{escape_quote(value)}";')
        elif type.assignable_to(FloatType()):
            self.print(f"{name}: {value};")
        elif type.assignable_to(BoolType()):
            val = truthy(value)
            self.print(f"{name}: {'true' if val else 'false'};")
        elif (
            type.assignable_to(self.gir.namespaces["Gtk"].lookup_type("Gdk.Pixbuf"))
            or type.assignable_to(self.gir.namespaces["Gtk"].lookup_type("Gdk.Texture"))
            or type.assignable_to(
                self.gir.namespaces["Gtk"].lookup_type("Gdk.Paintable")
            )
            or type.assignable_to(
                self.gir.namespaces["Gtk"].lookup_type("Gtk.ShortcutAction")
            )
            or type.assignable_to(
                self.gir.namespaces["Gtk"].lookup_type("Gtk.ShortcutTrigger")
            )
        ):
            self.print(f'{name}: "{escape_quote(value)}";')
        elif value == self.template_class:
            self.print(f"{name}: template;")
        elif type.assignable_to(
            self.gir.namespaces["Gtk"].lookup_type("GObject.Object")
        ):
            self.print(f"{name}: {value};")
        elif isinstance(type, Bitfield):
            flags = [get_enum_name(flag) for flag in value.split("|")]
            self.print(f"{name}: {' | '.join(flags)};")
        elif isinstance(type, Enumeration):
            self.print(f"{name}: {get_enum_name(value)};")
        else:
            self.print(f'{name}: "{escape_quote(value)}";')


def _decompile_element(
    ctx: DecompileCtx, gir: T.Optional[GirContext], xml: Element
) -> None:
    try:
        decompiler = _DECOMPILERS.get(xml.tag)
        if decompiler is None:
            raise UnsupportedError(f"unsupported XML tag: <{xml.tag}>")

        args: T.Dict[str, T.Optional[str]] = {
            canon(name): value for name, value in xml.attrs.items()
        }
        if decompiler._cdata:
            if len(xml.children):
                args["cdata"] = None
            else:
                args["cdata"] = xml.cdata

        ctx.start_block()
        gir = decompiler(ctx, gir, **args)

        for child in xml.children:
            _decompile_element(ctx, gir, child)

        ctx.end_block()

    except UnsupportedError as e:
        raise e
    except TypeError as e:
        raise UnsupportedError(tag=xml.tag)


def decompile(data: str) -> str:
    ctx = DecompileCtx()

    xml = parse(data)
    _decompile_element(ctx, None, xml)

    return ctx.result


def decompile_string(data):
    ctx = DecompileCtx()

    xml = parse_string(data)
    _decompile_element(ctx, None, xml)

    return ctx.result


def canon(string: str) -> str:
    if string == "class":
        return "klass"
    else:
        return string.replace("-", "_").lower()


def truthy(string: str) -> bool:
    return string.lower() in ["yes", "true", "t", "y", "1"]


def full_name(gir) -> str:
    return gir.name if gir.full_name.startswith("Gtk.") else gir.full_name


def lookup_by_cname(gir, cname: str) -> T.Optional[GirType]:
    if isinstance(gir, GirContext):
        return gir.get_type_by_cname(cname)
    else:
        return gir.get_containing(Repository).get_type_by_cname(cname)


def decompiler(tag, cdata=False):
    def decorator(func):
        func._cdata = cdata
        _DECOMPILERS[tag] = func
        return func

    return decorator


def escape_quote(string: str) -> str:
    return (
        string.replace("\\", "\\\\")
        .replace("'", "\\'")
        .replace('"', '\\"')
        .replace("\n", "\\n")
    )


@decompiler("interface")
def decompile_interface(ctx, gir):
    return gir


@decompiler("requires")
def decompile_requires(ctx, gir, lib=None, version=None):
    return gir


@decompiler("placeholder")
def decompile_placeholder(ctx, gir):
    pass


def decompile_translatable(
    string: str,
    translatable: T.Optional[str],
    context: T.Optional[str],
    comments: T.Optional[str],
) -> T.Tuple[T.Optional[str], str]:
    if translatable is not None and truthy(translatable):
        if comments is not None:
            comments = comments.replace("/*", " ").replace("*/", " ")
            comments = f"/* Translators: {comments} */"

        if context is not None:
            return comments, f'C_("{escape_quote(context)}", "{escape_quote(string)}")'
        else:
            return comments, f'_("{escape_quote(string)}")'
    else:
        return comments, f'"{escape_quote(string)}"'


@decompiler("property", cdata=True)
def decompile_property(
    ctx: DecompileCtx,
    gir,
    name,
    cdata,
    bind_source=None,
    bind_property=None,
    bind_flags=None,
    translatable="false",
    comments=None,
    context=None,
):
    name = name.replace("_", "-")
    if comments is not None:
        ctx.print(f"/* Translators: {comments} */")

    if cdata is None:
        ctx.print(f"{name}: ", False)
        ctx.end_block_with(";")
    elif bind_source:
        flags = ""
        bind_flags = bind_flags or []
        if "sync-create" not in bind_flags:
            flags += " no-sync-create"
        if "invert-boolean" in bind_flags:
            flags += " inverted"
        if "bidirectional" in bind_flags:
            flags += " bidirectional"
        ctx.print(f"{name}: bind-property {bind_source}.{bind_property}{flags};")
    elif truthy(translatable):
        comments, translatable = decompile_translatable(
            cdata, translatable, context, comments
        )
        if comments is not None:
            ctx.print(comments)
        ctx.print(f"{name}: {translatable};")
    elif gir is None or gir.properties.get(name) is None:
        ctx.print(f'{name}: "{escape_quote(cdata)}";')
    else:
        ctx.print_attribute(name, cdata, gir.properties.get(name).type)
    return gir


@decompiler("attribute", cdata=True)
def decompile_attribute(
    ctx, gir, name, cdata, translatable="false", comments=None, context=None
):
    decompile_property(
        ctx,
        gir,
        name,
        cdata,
        translatable=translatable,
        comments=comments,
        context=context,
    )


@decompiler("attributes")
def decompile_attributes(ctx, gir):
    ctx.print("attributes {")


@dataclass
class UnsupportedError(Exception):
    message: str = "unsupported feature"
    tag: T.Optional[str] = None

    def print(self, filename: str):
        print(f"\n{Colors.RED}{Colors.BOLD}error: {self.message}{Colors.CLEAR}")
        print(f"in {Colors.UNDERLINE}{filename}{Colors.NO_UNDERLINE}")
        if self.tag:
            print(f"in tag {Colors.BLUE}{self.tag}{Colors.CLEAR}")
        print(
            f"""{Colors.FAINT}The compiler might support this feature, but the porting tool does not. You
probably need to port this file manually.{Colors.CLEAR}\n"""
        )
