# imports.py
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


from .. import gir
from .common import *


class GtkDirective(AstNode):
    grammar = Statement(
        Match("using").err(
            'File must start with a "using Gtk" directive (e.g. `using Gtk 4.0;`)'
        ),
        Match("Gtk").err(
            'File must start with a "using Gtk" directive (e.g. `using Gtk 4.0;`)'
        ),
        UseNumberText("version").expected("a version number for GTK"),
    )

    @validate("version")
    def gtk_version(self):
        version = self.tokens["version"]
        if version not in ["4.0"]:
            err = CompileError("Only GTK 4 is supported")
            if version and version.startswith("4"):
                err.hint(
                    "Expected the GIR version, not an exact version number. Use 'using Gtk 4.0;'."
                )
            else:
                err.hint("Expected 'using Gtk 4.0;'")
            raise err

        try:
            gir.get_namespace("Gtk", version)
        except CompileError as e:
            raise CompileError(
                "Could not find GTK 4 introspection files. Is gobject-introspection installed?",
                fatal=True,
                # preserve the hints from the original error, because it contains
                # useful debugging information
                hints=e.hints,
            )

    @property
    def gir_namespace(self):
        # validate the GTK version first to make sure the more specific error
        # message is emitted
        self.gtk_version()
        if self.tokens["version"] is not None:
            return gir.get_namespace("Gtk", self.tokens["version"])
        else:
            # For better error handling, just assume it's 4.0
            return gir.get_namespace("Gtk", "4.0")


class Import(AstNode):
    grammar = Statement(
        "using",
        UseIdent("namespace").expected("a GIR namespace"),
        UseNumberText("version").expected("a version number"),
    )

    @property
    def namespace(self):
        return self.tokens["namespace"]

    @property
    def version(self):
        return self.tokens["version"]

    @validate("namespace", "version")
    def namespace_exists(self):
        gir.get_namespace(self.tokens["namespace"], self.tokens["version"])

    @property
    def gir_namespace(self):
        try:
            return gir.get_namespace(self.tokens["namespace"], self.tokens["version"])
        except CompileError:
            return None
