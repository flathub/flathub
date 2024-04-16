# gir.py
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

import os
import sys
import typing as T
from functools import cached_property

import gi  # type: ignore

gi.require_version("GIRepository", "2.0")
from gi.repository import GIRepository  # type: ignore

from . import typelib, xml_reader
from .errors import CompileError, CompilerBugError

_namespace_cache: T.Dict[str, "Namespace"] = {}
_xml_cache = {}

_user_search_paths = []


def add_typelib_search_path(path: str):
    _user_search_paths.append(path)


def get_namespace(namespace: str, version: str) -> "Namespace":
    search_paths = [*GIRepository.Repository.get_search_path(), *_user_search_paths]

    filename = f"{namespace}-{version}.typelib"

    if filename not in _namespace_cache:
        for search_path in search_paths:
            path = os.path.join(search_path, filename)

            if os.path.exists(path) and os.path.isfile(path):
                tl = typelib.load_typelib(path)
                repository = Repository(tl)

                _namespace_cache[filename] = repository.namespace
                break

        if filename not in _namespace_cache:
            raise CompileError(
                f"Namespace {namespace}-{version} could not be found",
                hints=["search path: " + os.pathsep.join(search_paths)],
            )

    return _namespace_cache[filename]


def get_xml(namespace: str, version: str):
    search_paths = []

    if data_paths := os.environ.get("XDG_DATA_DIRS"):
        search_paths += [
            os.path.join(path, "gir-1.0") for path in data_paths.split(os.pathsep)
        ]

    filename = f"{namespace}-{version}.gir"

    if filename not in _xml_cache:
        for search_path in search_paths:
            path = os.path.join(search_path, filename)

            if os.path.exists(path) and os.path.isfile(path):
                _xml_cache[filename] = xml_reader.parse(path)
                break

        if filename not in _xml_cache:
            raise CompileError(
                f"GObject introspection file '{namespace}-{version}.gir' could not be found",
                hints=["search path: " + os.pathsep.join(search_paths)],
            )

    return _xml_cache[filename]


ONLINE_DOCS = {
    "Adw-1": "https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/",
    "Gdk-4.0": "https://docs.gtk.org/gdk4/",
    "GdkPixbuf-2.0": "https://docs.gtk.org/gdk-pixbuf/",
    "Gio-2.0": "https://docs.gtk.org/gio/",
    "GLib-2.0": "https://docs.gtk.org/glib/",
    "GModule-2.0": "https://docs.gtk.org/gmodule/",
    "GObject-2.0": "https://docs.gtk.org/gobject/",
    "Gsk-4.0": "https://docs.gtk.org/gsk4/",
    "Gtk-4.0": "https://docs.gtk.org/gtk4/",
    "GtkSource-5": "https://gnome.pages.gitlab.gnome.org/gtksourceview/gtksourceview5",
    "Pango-1.0": "https://docs.gtk.org/Pango/",
    "Shumate-1.0": "https://gnome.pages.gitlab.gnome.org/libshumate/",
    "WebKit2-4.1": "https://webkitgtk.org/reference/webkit2gtk/stable/",
}


class GirType:
    @property
    def doc(self) -> T.Optional[str]:
        return None

    def assignable_to(self, other: "GirType") -> bool:
        raise NotImplementedError()

    @property
    def name(self) -> str:
        """The GIR name of the type, not including the namespace"""
        raise NotImplementedError()

    @property
    def full_name(self) -> str:
        """The GIR name of the type to use in diagnostics"""
        raise NotImplementedError()

    @property
    def glib_type_name(self) -> str:
        """The name of the type in the GObject type system, suitable to pass to `g_type_from_name()`."""
        raise NotImplementedError()

    @property
    def incomplete(self) -> bool:
        return False


class ExternType(GirType):
    def __init__(self, name: str) -> None:
        super().__init__()
        self._name = name

    def assignable_to(self, other: GirType) -> bool:
        return True

    @property
    def full_name(self) -> str:
        return self._name

    @property
    def glib_type_name(self) -> str:
        return self._name

    @property
    def incomplete(self) -> bool:
        return True


class ArrayType(GirType):
    def __init__(self, inner: GirType) -> None:
        self._inner = inner

    def assignable_to(self, other: GirType) -> bool:
        return isinstance(other, ArrayType) and self._inner.assignable_to(other._inner)

    @property
    def name(self) -> str:
        return self._inner.name + "[]"

    @property
    def full_name(self) -> str:
        return self._inner.full_name + "[]"


class BasicType(GirType):
    name: str = "unknown type"

    @property
    def full_name(self) -> str:
        return self.name


class BoolType(BasicType):
    name = "bool"
    glib_type_name: str = "gboolean"

    def assignable_to(self, other: GirType) -> bool:
        return isinstance(other, BoolType)


class IntType(BasicType):
    name = "int"
    glib_type_name: str = "gint"

    def assignable_to(self, other: GirType) -> bool:
        return (
            isinstance(other, IntType)
            or isinstance(other, UIntType)
            or isinstance(other, FloatType)
        )


class UIntType(BasicType):
    name = "uint"
    glib_type_name: str = "guint"

    def assignable_to(self, other: GirType) -> bool:
        return (
            isinstance(other, IntType)
            or isinstance(other, UIntType)
            or isinstance(other, FloatType)
        )


class FloatType(BasicType):
    name = "float"
    glib_type_name: str = "gfloat"

    def assignable_to(self, other: GirType) -> bool:
        return isinstance(other, FloatType)


class StringType(BasicType):
    name = "string"
    glib_type_name: str = "gchararray"

    def assignable_to(self, other: GirType) -> bool:
        return isinstance(other, StringType)


class TypeType(BasicType):
    name = "GType"
    glib_type_name: str = "GType"

    def assignable_to(self, other: GirType) -> bool:
        return isinstance(other, TypeType)


_BASIC_TYPES = {
    "bool": BoolType,
    "string": StringType,
    "int": IntType,
    "uint": UIntType,
    "float": FloatType,
    "double": FloatType,
    "type": TypeType,
}


TNode = T.TypeVar("TNode", bound="GirNode")


class GirNode:
    xml_tag: str

    def __init__(self, container: T.Optional["GirNode"], tl: typelib.Typelib) -> None:
        self.container = container
        self.tl = tl

    def get_containing(self, container_type: T.Type[TNode]) -> TNode:
        if self.container is None:
            raise CompilerBugError()
        elif isinstance(self.container, container_type):
            return self.container
        else:
            return self.container.get_containing(container_type)

    @cached_property
    def xml(self):
        for el in self.container.xml.children:
            if el.attrs.get("name") == self.name:
                if el.tag == self.xml_tag:
                    return el

    @cached_property
    def glib_type_name(self) -> str:
        return self.tl.OBJ_GTYPE_NAME

    @cached_property
    def full_name(self) -> str:
        if self.container is None:
            return self.name
        else:
            return f"{self.container.name}.{self.name}"

    @cached_property
    def name(self) -> str:
        return self.tl.BLOB_NAME

    @cached_property
    def cname(self) -> str:
        return self.tl.OBJ_GTYPE_NAME

    @cached_property
    def available_in(self) -> str:
        return self.xml.get("version")

    @cached_property
    def doc(self) -> T.Optional[str]:
        sections = []

        if self.signature:
            sections.append("```\n" + self.signature + "\n```")

        try:
            el = self.xml.get_elements("doc")
            if len(el) == 1:
                sections.append(el[0].cdata.strip())
        except:
            # Not a huge deal, but if you want docs in the language server you
            # should ensure .gir files are installed
            sections.append("Documentation is not installed")

        if self.online_docs:
            sections.append(f"[Online documentation]({self.online_docs})")

        return "\n\n---\n\n".join(sections)

    @property
    def online_docs(self) -> T.Optional[str]:
        return None

    @property
    def signature(self) -> T.Optional[str]:
        return None

    @property
    def type(self) -> GirType:
        raise NotImplementedError()


class Property(GirNode):
    xml_tag = "property"

    def __init__(self, klass: T.Union["Class", "Interface"], tl: typelib.Typelib):
        super().__init__(klass, tl)

    @cached_property
    def name(self) -> str:
        return self.tl.PROP_NAME

    @cached_property
    def type(self):
        return self.get_containing(Repository)._resolve_type_id(self.tl.PROP_TYPE)

    @cached_property
    def signature(self):
        return f"{self.type.full_name} {self.container.name}:{self.name}"

    @property
    def writable(self) -> bool:
        return self.tl.PROP_WRITABLE == 1

    @property
    def construct_only(self) -> bool:
        return self.tl.PROP_CONSTRUCT_ONLY == 1

    @property
    def online_docs(self) -> T.Optional[str]:
        if ns := self.get_containing(Namespace).online_docs:
            assert self.container is not None
            return f"{ns}property.{self.container.name}.{self.name}.html"
        else:
            return None


class Argument(GirNode):
    def __init__(self, container: GirNode, tl: typelib.Typelib) -> None:
        super().__init__(container, tl)

    @cached_property
    def name(self) -> str:
        return self.tl.ARG_NAME

    @cached_property
    def type(self) -> GirType:
        return self.get_containing(Repository)._resolve_type_id(self.tl.ARG_TYPE)


class Signature(GirNode):
    def __init__(self, container: GirNode, tl: typelib.Typelib) -> None:
        super().__init__(container, tl)

    @cached_property
    def args(self) -> T.List[Argument]:
        n_arguments = self.tl.SIGNATURE_N_ARGUMENTS
        blob_size = self.tl.header.HEADER_ARG_BLOB_SIZE
        result = []
        for i in range(n_arguments):
            entry = self.tl.SIGNATURE_ARGUMENTS[i * blob_size]
            result.append(Argument(self, entry))
        return result

    @cached_property
    def return_type(self) -> GirType:
        return self.get_containing(Repository)._resolve_type_id(
            self.tl.SIGNATURE_RETURN_TYPE
        )


class Signal(GirNode):
    xml_tag = "glib:signal"

    def __init__(
        self, klass: T.Union["Class", "Interface"], tl: typelib.Typelib
    ) -> None:
        super().__init__(klass, tl)

    @cached_property
    def gir_signature(self) -> Signature:
        return Signature(self, self.tl.SIGNAL_SIGNATURE)

    @property
    def signature(self):
        args = ", ".join(
            [f"{a.type.full_name} {a.name}" for a in self.gir_signature.args]
        )
        return f"signal {self.container.full_name}::{self.name} ({args})"

    @property
    def online_docs(self) -> T.Optional[str]:
        if ns := self.get_containing(Namespace).online_docs:
            assert self.container is not None
            return f"{ns}signal.{self.container.name}.{self.name}.html"
        else:
            return None


class Interface(GirNode, GirType):
    xml_tag = "interface"

    def __init__(self, ns: "Namespace", tl: typelib.Typelib):
        super().__init__(ns, tl)

    @cached_property
    def properties(self) -> T.Mapping[str, Property]:
        n_prerequisites = self.tl.INTERFACE_N_PREREQUISITES
        offset = self.tl.header.HEADER_INTERFACE_BLOB_SIZE
        offset += (n_prerequisites + n_prerequisites % 2) * 2
        n_properties = self.tl.INTERFACE_N_PROPERTIES
        property_size = self.tl.header.HEADER_PROPERTY_BLOB_SIZE
        result = {}
        for i in range(n_properties):
            property = Property(self, self.tl[offset + i * property_size])
            result[property.name] = property
        return result

    @cached_property
    def signals(self) -> T.Mapping[str, Signal]:
        n_prerequisites = self.tl.INTERFACE_N_PREREQUISITES
        offset = self.tl.header.HEADER_INTERFACE_BLOB_SIZE
        offset += (n_prerequisites + n_prerequisites % 2) * 2
        offset += (
            self.tl.INTERFACE_N_PROPERTIES * self.tl.header.HEADER_PROPERTY_BLOB_SIZE
        )
        offset += self.tl.INTERFACE_N_METHODS * self.tl.header.HEADER_FUNCTION_BLOB_SIZE
        n_signals = self.tl.INTERFACE_N_SIGNALS
        property_size = self.tl.header.HEADER_SIGNAL_BLOB_SIZE
        result = {}
        for i in range(n_signals):
            signal = Signal(self, self.tl[offset + i * property_size])
            result[signal.name] = signal
        return result

    @cached_property
    def prerequisites(self) -> T.List["Interface"]:
        n_prerequisites = self.tl.INTERFACE_N_PREREQUISITES
        result = []
        for i in range(n_prerequisites):
            entry = self.tl.INTERFACE_PREREQUISITES[i * 2].AS_DIR_ENTRY
            result.append(self.get_containing(Repository)._resolve_dir_entry(entry))
        return result

    def assignable_to(self, other: GirType) -> bool:
        if self == other:
            return True
        for pre in self.prerequisites:
            if pre.assignable_to(other):
                return True
        return False

    @property
    def online_docs(self) -> T.Optional[str]:
        if ns := self.get_containing(Namespace).online_docs:
            return f"{ns}interface.{self.name}.html"
        else:
            return None


class Class(GirNode, GirType):
    xml_tag = "class"

    def __init__(self, ns: "Namespace", tl: typelib.Typelib) -> None:
        super().__init__(ns, tl)

    @property
    def abstract(self) -> bool:
        return self.tl.OBJ_ABSTRACT == 1

    @cached_property
    def implements(self) -> T.List[Interface]:
        n_interfaces = self.tl.OBJ_N_INTERFACES
        result = []
        for i in range(n_interfaces):
            entry = self.tl[self.tl.header.HEADER_OBJECT_BLOB_SIZE + i * 2].AS_DIR_ENTRY
            result.append(self.get_containing(Repository)._resolve_dir_entry(entry))
        return result

    @cached_property
    def own_properties(self) -> T.Mapping[str, Property]:
        n_interfaces = self.tl.OBJ_N_INTERFACES
        offset = self.tl.header.HEADER_OBJECT_BLOB_SIZE
        offset += (n_interfaces + n_interfaces % 2) * 2
        offset += self.tl.OBJ_N_FIELDS * self.tl.header.HEADER_FIELD_BLOB_SIZE
        offset += (
            self.tl.OBJ_N_FIELD_CALLBACKS * self.tl.header.HEADER_CALLBACK_BLOB_SIZE
        )
        n_properties = self.tl.OBJ_N_PROPERTIES
        property_size = self.tl.header.HEADER_PROPERTY_BLOB_SIZE
        result = {}
        for i in range(n_properties):
            property = Property(self, self.tl[offset + i * property_size])
            result[property.name] = property
        return result

    @cached_property
    def own_signals(self) -> T.Mapping[str, Signal]:
        n_interfaces = self.tl.OBJ_N_INTERFACES
        offset = self.tl.header.HEADER_OBJECT_BLOB_SIZE
        offset += (n_interfaces + n_interfaces % 2) * 2
        offset += self.tl.OBJ_N_FIELDS * self.tl.header.HEADER_FIELD_BLOB_SIZE
        offset += (
            self.tl.OBJ_N_FIELD_CALLBACKS * self.tl.header.HEADER_CALLBACK_BLOB_SIZE
        )
        offset += self.tl.OBJ_N_PROPERTIES * self.tl.header.HEADER_PROPERTY_BLOB_SIZE
        offset += self.tl.OBJ_N_METHODS * self.tl.header.HEADER_FUNCTION_BLOB_SIZE
        n_signals = self.tl.OBJ_N_SIGNALS
        signal_size = self.tl.header.HEADER_SIGNAL_BLOB_SIZE
        result = {}
        for i in range(n_signals):
            signal = Signal(self, self.tl[offset][i * signal_size])
            result[signal.name] = signal
        return result

    @cached_property
    def parent(self) -> T.Optional["Class"]:
        if entry := self.tl.OBJ_PARENT:
            return self.get_containing(Repository)._resolve_dir_entry(entry)
        else:
            return None

    @cached_property
    def signature(self) -> str:
        assert self.container is not None
        result = f"class {self.container.name}.{self.name}"
        if self.parent is not None:
            assert self.parent.container is not None
            result += f" : {self.parent.container.name}.{self.parent.name}"
        if len(self.implements):
            result += " implements " + ", ".join(
                [impl.full_name for impl in self.implements]
            )
        return result

    @cached_property
    def properties(self) -> T.Mapping[str, Property]:
        return {p.name: p for p in self._enum_properties()}

    @cached_property
    def signals(self) -> T.Mapping[str, Signal]:
        return {s.name: s for s in self._enum_signals()}

    def assignable_to(self, other: GirType) -> bool:
        if self == other:
            return True
        elif self.parent and self.parent.assignable_to(other):
            return True
        else:
            for iface in self.implements:
                if iface.assignable_to(other):
                    return True

            return False

    def _enum_properties(self) -> T.Iterable[Property]:
        yield from self.own_properties.values()

        if self.parent is not None:
            yield from self.parent.properties.values()

        for impl in self.implements:
            yield from impl.properties.values()

    def _enum_signals(self) -> T.Iterable[Signal]:
        yield from self.own_signals.values()

        if self.parent is not None:
            yield from self.parent.signals.values()

        for impl in self.implements:
            yield from impl.signals.values()

    @property
    def online_docs(self) -> T.Optional[str]:
        if ns := self.get_containing(Namespace).online_docs:
            return f"{ns}class.{self.name}.html"
        else:
            return None


class TemplateType(GirType):
    def __init__(self, name: str, parent: T.Optional[GirType]):
        self._name = name
        self.parent = parent

    @property
    def name(self) -> str:
        return self._name

    @property
    def full_name(self) -> str:
        return self._name

    @property
    def glib_type_name(self) -> str:
        return self._name

    @cached_property
    def properties(self) -> T.Mapping[str, Property]:
        if not (isinstance(self.parent, Class) or isinstance(self.parent, Interface)):
            return {}
        else:
            return self.parent.properties

    @cached_property
    def signals(self) -> T.Mapping[str, Signal]:
        if not (isinstance(self.parent, Class) or isinstance(self.parent, Interface)):
            return {}
        else:
            return self.parent.signals

    def assignable_to(self, other: "GirType") -> bool:
        if self == other:
            return True
        elif isinstance(other, Interface):
            # we don't know the template type's interfaces, assume yes
            return True
        elif self.parent is None or isinstance(self.parent, ExternType):
            return isinstance(other, Class)
        else:
            return self.parent.assignable_to(other)

    @cached_property
    def signature(self) -> str:
        if self.parent is None:
            return f"template {self.name}"
        else:
            return f"template {self.name} : {self.parent.full_name}"

    @property
    def incomplete(self) -> bool:
        return True


class EnumMember(GirNode):
    xml_tag = "member"

    def __init__(self, enum: "Enumeration", tl: typelib.Typelib) -> None:
        super().__init__(enum, tl)

    @property
    def value(self) -> int:
        return self.tl.VALUE_VALUE

    @cached_property
    def name(self) -> str:
        return self.tl.VALUE_NAME

    @cached_property
    def nick(self) -> str:
        return self.name.replace("_", "-")

    @property
    def c_ident(self) -> str:
        return self.tl.attr("c:identifier")

    @property
    def signature(self) -> str:
        return f"enum member {self.full_name} = {self.value}"


class Enumeration(GirNode, GirType):
    xml_tag = "enumeration"

    def __init__(self, ns: "Namespace", tl: typelib.Typelib) -> None:
        super().__init__(ns, tl)

    @cached_property
    def members(self) -> T.Dict[str, EnumMember]:
        members = {}
        n_values = self.tl.ENUM_N_VALUES
        values = self.tl.ENUM_VALUES
        value_size = self.tl.header.HEADER_VALUE_BLOB_SIZE
        for i in range(n_values):
            member = EnumMember(self, values[i * value_size])
            members[member.name] = member
        return members

    @property
    def signature(self) -> str:
        return f"enum {self.full_name}"

    def assignable_to(self, type: GirType) -> bool:
        return type == self

    @property
    def online_docs(self) -> T.Optional[str]:
        if ns := self.get_containing(Namespace).online_docs:
            return f"{ns}enum.{self.name}.html"
        else:
            return None


class Boxed(GirNode, GirType):
    xml_tag = "glib:boxed"

    def __init__(self, ns: "Namespace", tl: typelib.Typelib) -> None:
        super().__init__(ns, tl)

    @property
    def signature(self) -> str:
        return f"boxed {self.full_name}"

    def assignable_to(self, type) -> bool:
        return type == self

    @property
    def online_docs(self) -> T.Optional[str]:
        if ns := self.get_containing(Namespace).online_docs:
            return f"{ns}boxed.{self.name}.html"
        else:
            return None


class Bitfield(Enumeration):
    xml_tag = "bitfield"

    def __init__(self, ns: "Namespace", tl: typelib.Typelib) -> None:
        super().__init__(ns, tl)


class Namespace(GirNode):
    def __init__(self, repo: "Repository", tl: typelib.Typelib) -> None:
        super().__init__(repo, tl)

    @cached_property
    def entries(self) -> T.Mapping[str, GirType]:
        entries: dict[str, GirType] = {}

        n_local_entries: int = self.tl.HEADER_N_ENTRIES
        directory: typelib.Typelib = self.tl.HEADER_DIRECTORY
        blob_size: int = self.tl.header.HEADER_ENTRY_BLOB_SIZE

        for i in range(n_local_entries):
            entry = directory[i * blob_size]
            entry_name: str = entry.DIR_ENTRY_NAME
            entry_type: int = entry.DIR_ENTRY_BLOB_TYPE
            entry_blob: typelib.Typelib = entry.DIR_ENTRY_OFFSET

            if entry_type == typelib.BLOB_TYPE_ENUM:
                entries[entry_name] = Enumeration(self, entry_blob)
            elif entry_type == typelib.BLOB_TYPE_FLAGS:
                entries[entry_name] = Bitfield(self, entry_blob)
            elif entry_type == typelib.BLOB_TYPE_OBJECT:
                entries[entry_name] = Class(self, entry_blob)
            elif entry_type == typelib.BLOB_TYPE_INTERFACE:
                entries[entry_name] = Interface(self, entry_blob)
            elif (
                entry_type == typelib.BLOB_TYPE_BOXED
                or entry_type == typelib.BLOB_TYPE_STRUCT
            ):
                entries[entry_name] = Boxed(self, entry_blob)

        return entries

    @cached_property
    def xml(self):
        return get_xml(self.name, self.version).get_elements("namespace")[0]

    @cached_property
    def name(self) -> str:
        return self.tl.HEADER_NAMESPACE

    @cached_property
    def version(self) -> str:
        return self.tl.HEADER_NSVERSION

    @property
    def signature(self) -> str:
        return f"namespace {self.name} {self.version}"

    @cached_property
    def classes(self) -> T.Mapping[str, Class]:
        return {
            name: entry
            for name, entry in self.entries.items()
            if isinstance(entry, Class)
        }

    @cached_property
    def interfaces(self) -> T.Mapping[str, Interface]:
        return {
            name: entry
            for name, entry in self.entries.items()
            if isinstance(entry, Interface)
        }

    def get_type(self, name) -> T.Optional[GirType]:
        """Gets a type (class, interface, enum, etc.) from this namespace."""
        return self.entries.get(name)

    def get_type_by_cname(self, cname: str) -> T.Optional[GirType]:
        """Gets a type from this namespace by its C name."""
        for item in self.entries.values():
            if hasattr(item, "cname") and item.cname == cname:
                return item
        return None

    def lookup_type(self, type_name: str) -> T.Optional[GirType]:
        """Looks up a type in the scope of this namespace (including in the
        namespace's dependencies)."""

        if type_name in _BASIC_TYPES:
            return _BASIC_TYPES[type_name]()
        elif "." in type_name:
            ns, name = type_name.split(".", 1)
            return self.get_containing(Repository).get_type(name, ns)
        else:
            return self.get_type(type_name)

    @property
    def online_docs(self) -> T.Optional[str]:
        return ONLINE_DOCS.get(f"{self.name}-{self.version}")


class Repository(GirNode):
    def __init__(self, tl: typelib.Typelib) -> None:
        super().__init__(None, tl)

        self.namespace = Namespace(self, tl)

        if dependencies := tl[0x24].string:
            deps = [tuple(dep.split("-", 1)) for dep in dependencies.split("|")]
            try:
                self.includes = {
                    name: get_namespace(name, version) for name, version in deps
                }
            except:
                raise CompilerBugError(f"Failed to load dependencies.")
        else:
            self.includes = {}

    def get_type(self, name: str, ns: str) -> T.Optional[GirType]:
        return self.lookup_namespace(ns).get_type(name)

    def get_type_by_cname(self, name: str) -> T.Optional[GirType]:
        for ns in [self.namespace, *self.includes.values()]:
            if type := ns.get_type_by_cname(name):
                return type
        return None

    def lookup_namespace(self, ns: str):
        """Finds a namespace among this namespace's dependencies."""
        if ns == self.namespace.name:
            return self.namespace
        else:
            for include in self.includes.values():
                if namespace := include.get_containing(Repository).lookup_namespace(ns):
                    return namespace

    def _resolve_dir_entry(self, dir_entry: typelib.Typelib):
        if dir_entry.DIR_ENTRY_LOCAL:
            return self.namespace.get_type(dir_entry.DIR_ENTRY_NAME)
        else:
            ns = dir_entry.DIR_ENTRY_NAMESPACE
            return self.lookup_namespace(ns).get_type(dir_entry.DIR_ENTRY_NAME)

    def _resolve_type_id(self, type_id: int) -> GirType:
        if type_id & 0xFFFFFF == 0:
            type_id = (type_id >> 27) & 0x1F
            # simple type
            if type_id == typelib.TYPE_BOOLEAN:
                return BoolType()
            elif type_id in [typelib.TYPE_FLOAT, typelib.TYPE_DOUBLE]:
                return FloatType()
            elif type_id in [
                typelib.TYPE_INT8,
                typelib.TYPE_INT16,
                typelib.TYPE_INT32,
                typelib.TYPE_INT64,
            ]:
                return IntType()
            elif type_id in [
                typelib.TYPE_UINT8,
                typelib.TYPE_UINT16,
                typelib.TYPE_UINT32,
                typelib.TYPE_UINT64,
            ]:
                return UIntType()
            elif type_id == typelib.TYPE_UTF8:
                return StringType()
            elif type_id == typelib.TYPE_GTYPE:
                return TypeType()
            else:
                raise CompilerBugError("Unknown type ID", type_id)
        else:
            blob = self.tl.header[type_id]
            if blob.TYPE_BLOB_TAG == typelib.TYPE_INTERFACE:
                return self._resolve_dir_entry(
                    self.tl.header[type_id].TYPE_BLOB_INTERFACE
                )
            elif blob.TYPE_BLOB_TAG == typelib.TYPE_ARRAY:
                return ArrayType(self._resolve_type_id(blob.TYPE_BLOB_ARRAY_INNER))
            else:
                raise CompilerBugError(f"{blob.TYPE_BLOB_TAG}")


class GirContext:
    def __init__(self):
        self.namespaces = {}
        self.not_found_namespaces: T.Set[str] = set()

    def add_namespace(self, namespace: Namespace):
        other = self.namespaces.get(namespace.name)
        if other is not None and other.version != namespace.version:
            raise CompileError(
                f"Namespace {namespace.name}-{namespace.version} can't be imported because version {other.version} was imported earlier"
            )

        self.namespaces[namespace.name] = namespace

    def get_type_by_cname(self, name: str) -> T.Optional[GirType]:
        for ns in self.namespaces.values():
            if type := ns.get_type_by_cname(name):
                return type
        return None

    def get_type(self, name: str, ns: str) -> T.Optional[GirType]:
        if ns is None and name in _BASIC_TYPES:
            return _BASIC_TYPES[name]()

        ns = ns or "Gtk"

        if ns not in self.namespaces:
            return None

        return self.namespaces[ns].get_type(name)

    def get_class(self, name: str, ns: str) -> T.Optional[Class]:
        type = self.get_type(name, ns)
        if isinstance(type, Class):
            return type
        else:
            return None

    def validate_ns(self, ns: str) -> None:
        """Raises an exception if there is a problem looking up the given
        namespace."""

        ns = ns or "Gtk"

        if ns not in self.namespaces and ns not in self.not_found_namespaces:
            raise CompileError(
                f"Namespace {ns} was not imported",
                did_you_mean=(ns, self.namespaces.keys()),
            )

    def validate_type(self, name: str, ns: str) -> None:
        """Raises an exception if there is a problem looking up the given type."""

        self.validate_ns(ns)

        type = self.get_type(name, ns)

        ns = ns or "Gtk"

        if type is None:
            raise CompileError(
                f"Namespace {ns} does not contain a type called {name}",
                did_you_mean=(name, self.namespaces[ns].classes.keys()),
            )
