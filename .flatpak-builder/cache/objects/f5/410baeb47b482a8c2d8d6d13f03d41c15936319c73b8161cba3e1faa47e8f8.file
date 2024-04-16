# interactive_port.py
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


import difflib
import os
import typing as T

from . import decompiler, parser, tokenizer
from .errors import CompilerBugError, MultipleErrors, PrintableError
from .outputs.xml import XmlOutput
from .utils import Colors

# A tool to interactively port projects to blueprints.


class CouldNotPort:
    def __init__(self, message: str):
        self.message = message


def change_suffix(f):
    return f.removesuffix(".ui") + ".blp"


def decompile_file(in_file, out_file) -> T.Union[str, CouldNotPort]:
    if os.path.exists(out_file):
        return CouldNotPort("already exists")

    try:
        decompiled = decompiler.decompile(in_file)

        try:
            # make sure the output compiles
            tokens = tokenizer.tokenize(decompiled)
            ast, errors, warnings = parser.parse(tokens)

            for warning in warnings:
                warning.pretty_print(out_file, decompiled)

            if errors:
                raise errors
            if not ast:
                raise CompilerBugError()

            output = XmlOutput()
            output.emit(ast)
        except PrintableError as e:
            e.pretty_print(out_file, decompiled)

            print(
                f"{Colors.RED}{Colors.BOLD}error: the generated file does not compile{Colors.CLEAR}"
            )
            print(f"in {Colors.UNDERLINE}{out_file}{Colors.NO_UNDERLINE}")
            print(
                f"""{Colors.FAINT}Either the original XML file had an error, or there is a bug in the
porting tool. If you think it's a bug (which is likely), please file an issue on GitLab:
{Colors.BLUE}{Colors.UNDERLINE}https://gitlab.gnome.org/jwestman/blueprint-compiler/-/issues/new?issue{Colors.CLEAR}\n"""
            )

            return CouldNotPort("does not compile")

        return decompiled

    except decompiler.UnsupportedError as e:
        e.print(in_file)
        return CouldNotPort("could not convert")


def listdir_recursive(subdir):
    files = os.listdir(subdir)
    for file in files:
        if file in ["_build", "build"]:
            continue
        if file.startswith("."):
            continue
        full = os.path.join(subdir, file)
        if full == "./subprojects":
            # skip the subprojects directory
            continue
        if os.path.isfile(full):
            yield full
        elif os.path.isdir(full):
            yield from listdir_recursive(full)


def yesno(prompt):
    while True:
        response = input(f"{Colors.BOLD}{prompt} [y/n] {Colors.CLEAR}")
        if response.lower() in ["yes", "y"]:
            return True
        elif response.lower() in ["no", "n"]:
            return False


def enter():
    input(f"{Colors.BOLD}Press Enter when you have done that: {Colors.CLEAR}")


def step1():
    print(
        f"{Colors.BOLD}STEP 1: Create subprojects/blueprint-compiler.wrap{Colors.CLEAR}"
    )

    if os.path.exists("subprojects/blueprint-compiler.wrap"):
        print("subprojects/blueprint-compiler.wrap already exists, skipping\n")
        return

    if yesno("Create subprojects/blueprint-compiler.wrap?"):
        try:
            os.mkdir("subprojects")
        except:
            pass

        from .main import VERSION

        VERSION = "main" if VERSION == "uninstalled" else "v" + VERSION

        with open("subprojects/blueprint-compiler.wrap", "w") as wrap:
            wrap.write(
                f"""[wrap-git]
directory = blueprint-compiler
url = https://gitlab.gnome.org/jwestman/blueprint-compiler.git
revision = {VERSION}
depth = 1

[provide]
program_names = blueprint-compiler"""
            )

    print()


def step2():
    print(f"{Colors.BOLD}STEP 2: Set up .gitignore{Colors.CLEAR}")

    if os.path.exists(".gitignore"):
        with open(".gitignore", "r+") as gitignore:
            ignored = [line.strip() for line in gitignore.readlines()]
            if "/subprojects/blueprint-compiler" not in ignored:
                if yesno("Add '/subprojects/blueprint-compiler' to .gitignore?"):
                    gitignore.write("\n/subprojects/blueprint-compiler\n")
            else:
                print(
                    "'/subprojects/blueprint-compiler' already in .gitignore, skipping"
                )
    else:
        if yesno("Create .gitignore with '/subprojects/blueprint-compiler'?"):
            with open(".gitignore", "w") as gitignore:
                gitignore.write("/subprojects/blueprint-compiler\n")

    print()


def step3():
    print(f"{Colors.BOLD}STEP 3: Convert UI files{Colors.CLEAR}")

    files = [
        (file, change_suffix(file), decompile_file(file, change_suffix(file)))
        for file in listdir_recursive(".")
        if file.endswith(".ui")
    ]

    success = 0
    for in_file, out_file, result in files:
        if isinstance(result, CouldNotPort):
            if result.message == "already exists":
                print(Colors.FAINT, end="")
            print(
                f"{Colors.RED}will not port {Colors.UNDERLINE}{in_file}{Colors.NO_UNDERLINE} -> {Colors.UNDERLINE}{out_file}{Colors.NO_UNDERLINE} [{result.message}]{Colors.CLEAR}"
            )
        else:
            print(
                f"will port {Colors.UNDERLINE}{in_file}{Colors.CLEAR} -> {Colors.UNDERLINE}{out_file}{Colors.CLEAR}"
            )
            success += 1

    print()
    if len(files) == 0:
        print(f"{Colors.RED}No UI files found.{Colors.CLEAR}")
    elif success == len(files):
        print(f"{Colors.GREEN}All files were converted.{Colors.CLEAR}")
    elif success > 0:
        print(
            f"{Colors.RED}{success} file(s) were converted, {len(files) - success} were not.{Colors.CLEAR}"
        )
    else:
        print(f"{Colors.RED}None of the files could be converted.{Colors.CLEAR}")

    if success > 0 and yesno("Save these changes?"):
        for in_file, out_file, result in files:
            if not isinstance(result, CouldNotPort):
                with open(out_file, "x") as file:
                    file.write(result)

    print()
    results = [
        (in_file, out_file)
        for in_file, out_file, result in files
        if not isinstance(result, CouldNotPort) or result.message == "already exists"
    ]
    if len(results):
        return zip(*results)
    else:
        return ([], [])


def step4(ported):
    print(f"{Colors.BOLD}STEP 4: Set up meson.build{Colors.CLEAR}")
    print(
        f"{Colors.BOLD}{Colors.YELLOW}NOTE: Depending on your build system setup, you may need to make some adjustments to this step.{Colors.CLEAR}"
    )

    meson_files = [
        file
        for file in listdir_recursive(".")
        if os.path.basename(file) == "meson.build"
    ]
    for meson_file in meson_files:
        with open(meson_file, "r") as f:
            if "gnome.compile_resources" in f.read():
                parent = os.path.dirname(meson_file)
                file_list = "\n    ".join(
                    [
                        f"'{os.path.relpath(file, parent)}',"
                        for file in ported
                        if file.startswith(parent)
                    ]
                )

                if len(file_list):
                    print(
                        f"{Colors.BOLD}Paste the following into {Colors.UNDERLINE}{meson_file}{Colors.NO_UNDERLINE}:{Colors.CLEAR}"
                    )
                    print(
                        f"""
blueprints = custom_target('blueprints',
  input: files(
    {file_list}
  ),
  output: '.',
  command: [find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'],
)
"""
                    )
                    enter()

                    print(
                        f"""{Colors.BOLD}Paste the following into the 'gnome.compile_resources()'
arguments in {Colors.UNDERLINE}{meson_file}{Colors.NO_UNDERLINE}:{Colors.CLEAR}

dependencies: blueprints,
    """
                    )
                    enter()

    print()


def step5(in_files):
    print(f"{Colors.BOLD}STEP 5: Update POTFILES.in{Colors.CLEAR}")

    if not os.path.exists("po/POTFILES.in"):
        print(
            f"{Colors.UNDERLINE}po/POTFILES.in{Colors.NO_UNDERLINE} does not exist, skipping\n"
        )
        return

    with open("po/POTFILES.in", "r") as potfiles:
        old_lines = potfiles.readlines()
        lines = old_lines.copy()
        for in_file in in_files:
            for i, line in enumerate(lines):
                if line.strip() == in_file.removeprefix("./"):
                    lines[i] = change_suffix(line.strip()) + "\n"

        new_data = "".join(lines)

    print(
        f"{Colors.BOLD}Will make the following changes to {Colors.UNDERLINE}po/POTFILES.in{Colors.CLEAR}"
    )
    print(
        "".join(
            [
                (
                    Colors.GREEN
                    if line.startswith("+")
                    else Colors.RED + Colors.FAINT
                    if line.startswith("-")
                    else ""
                )
                + line
                + Colors.CLEAR
                for line in difflib.unified_diff(old_lines, lines)
            ]
        )
    )

    if yesno("Is this ok?"):
        with open("po/POTFILES.in", "w") as potfiles:
            potfiles.writelines(lines)

    print()


def step6(in_files):
    print(f"{Colors.BOLD}STEP 6: Clean up{Colors.CLEAR}")

    if yesno("Delete old XML files?"):
        for file in in_files:
            try:
                os.remove(file)
            except:
                pass


def run(opts):
    step1()
    step2()
    in_files, out_files = step3()
    step4(out_files)
    step5(in_files)
    step6(in_files)

    print(
        f"{Colors.BOLD}STEP 6: Done! Make sure your app still builds and runs correctly.{Colors.CLEAR}"
    )
