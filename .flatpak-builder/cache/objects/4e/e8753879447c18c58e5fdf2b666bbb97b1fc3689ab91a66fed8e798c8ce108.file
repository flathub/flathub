# main.py
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


import argparse
import json
import os
import sys
import typing as T

from . import decompiler, interactive_port, parser, tokenizer
from .errors import CompilerBugError, MultipleErrors, PrintableError, report_bug
from .gir import add_typelib_search_path
from .lsp import LanguageServer
from .outputs import XmlOutput
from .utils import Colors

VERSION = "uninstalled"
LIBDIR = None


class BlueprintApp:
    def main(self):
        self.parser = argparse.ArgumentParser()
        self.subparsers = self.parser.add_subparsers(metavar="command")
        self.parser.set_defaults(func=self.cmd_help)

        compile = self.add_subcommand(
            "compile", "Compile blueprint files", self.cmd_compile
        )
        compile.add_argument("--output", dest="output", default="-")
        compile.add_argument("--typelib-path", nargs="?", action="append")
        compile.add_argument(
            "input", metavar="filename", default=sys.stdin, type=argparse.FileType("r")
        )

        batch_compile = self.add_subcommand(
            "batch-compile",
            "Compile many blueprint files at once",
            self.cmd_batch_compile,
        )
        batch_compile.add_argument("output_dir", metavar="output-dir")
        batch_compile.add_argument("input_dir", metavar="input-dir")
        batch_compile.add_argument("--typelib-path", nargs="?", action="append")
        batch_compile.add_argument(
            "inputs",
            nargs="+",
            metavar="filenames",
            default=sys.stdin,
            type=argparse.FileType("r"),
        )

        port = self.add_subcommand("port", "Interactive porting tool", self.cmd_port)

        lsp = self.add_subcommand(
            "lsp", "Run the language server (for internal use by IDEs)", self.cmd_lsp
        )

        self.add_subcommand("help", "Show this message", self.cmd_help)

        self.parser.add_argument("--version", action="version", version=VERSION)

        try:
            opts = self.parser.parse_args()
            opts.func(opts)
        except SystemExit as e:
            raise e
        except KeyboardInterrupt:
            print(f"\n\n{Colors.RED}{Colors.BOLD}Interrupted.{Colors.CLEAR}")
        except EOFError:
            print(f"\n\n{Colors.RED}{Colors.BOLD}Interrupted.{Colors.CLEAR}")
        except:
            report_bug()

    def add_subcommand(self, name: str, help: str, func):
        parser = self.subparsers.add_parser(name, help=help)
        parser.set_defaults(func=func)
        return parser

    def cmd_help(self, opts):
        self.parser.print_help()

    def cmd_compile(self, opts):
        if opts.typelib_path != None:
            for typelib_path in opts.typelib_path:
                add_typelib_search_path(typelib_path)

        data = opts.input.read()
        try:
            xml, warnings = self._compile(data)

            for warning in warnings:
                warning.pretty_print(opts.input.name, data, stream=sys.stderr)

            if opts.output == "-":
                print(xml)
            else:
                with open(opts.output, "w") as file:
                    file.write(xml)
        except PrintableError as e:
            e.pretty_print(opts.input.name, data, stream=sys.stderr)
            sys.exit(1)

    def cmd_batch_compile(self, opts):
        if opts.typelib_path != None:
            for typelib_path in opts.typelib_path:
                add_typelib_search_path(typelib_path)

        for file in opts.inputs:
            data = file.read()

            try:
                if not os.path.commonpath([file.name, opts.input_dir]):
                    print(
                        f"{Colors.RED}{Colors.BOLD}error: input file '{file.name}' is not in input directory '{opts.input_dir}'{Colors.CLEAR}"
                    )
                    sys.exit(1)

                xml, warnings = self._compile(data)

                for warning in warnings:
                    warning.pretty_print(file.name, data, stream=sys.stderr)

                path = os.path.join(
                    opts.output_dir,
                    os.path.relpath(
                        os.path.splitext(file.name)[0] + ".ui", opts.input_dir
                    ),
                )
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, "w") as file:
                    file.write(xml)
            except PrintableError as e:
                e.pretty_print(file.name, data)
                sys.exit(1)

    def cmd_lsp(self, opts):
        langserv = LanguageServer()
        langserv.run()

    def cmd_port(self, opts):
        interactive_port.run(opts)

    def _compile(self, data: str) -> T.Tuple[str, T.List[PrintableError]]:
        tokens = tokenizer.tokenize(data)
        ast, errors, warnings = parser.parse(tokens)

        if errors:
            raise errors
        if ast is None:
            raise CompilerBugError()

        formatter = XmlOutput()

        return formatter.emit(ast), warnings


def main(version, libdir):
    global VERSION, LIBDIR
    VERSION, LIBDIR = version, libdir
    BlueprintApp().main()
