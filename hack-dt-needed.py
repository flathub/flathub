#!/usr/bin/env python3

'''
Who needs -Wl,--as-needed when you have Python?
'''

# Copyright © 2021 Collabora Ltd.
#
# SPDX-License-Identifier: MIT
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import argparse
import os
from typing import (
    Any,
    Dict,
    List,
)


class InvocationError(Exception):
    pass


MANY = 65536


class Main:
    def __init__(
        self,
        executables: List[str],
        **kwargs: Dict[str, Any],
    ) -> None:
        self.executables = executables

    def run(self) -> None:
        for exe in self.executables:
            with open(exe, 'rb') as reader, open(exe + '.new', 'wb') as writer:
                workspace = bytearray(0)

                while True:
                    buf = bytearray(MANY)
                    n = reader.readinto(buf)

                    if n == 0:
                        break

                    workspace = workspace + buf[:n]

                    self.replace_in_place(
                        workspace,
                        b'libQt5Svg.so.5',
                        b'libc.so.6',
                    )
                    self.replace_in_place(
                        workspace,
                        b'libavformat.so.57',
                        b'libc.so.6',
                    )
                    self.replace_in_place(
                        workspace,
                        b'libavresample.so.3',
                        b'libc.so.6',
                    )

                    if len(workspace) > 2 * MANY:
                        writer.write(workspace[:MANY])
                        workspace = workspace[MANY:]

                writer.write(workspace)

            os.chmod(exe + '.new', 0o755)
            os.rename(exe + '.new', exe)

    @staticmethod
    def replace_in_place(
        workspace: bytearray,
        original: bytes,
        replacement: bytes,
    ):
        assert len(replacement) < len(original)

        replacement = replacement + (b'\0' * len(original))
        replacement = replacement[:len(original)]

        i = 0

        while True:
            i = workspace.find(original, i)

            if i < 0:
                break

            workspace[i:i+len(original)] = replacement


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        'executables', metavar='EXECUTABLE', type=str, nargs='+',
    )

    try:
        args = parser.parse_args()
        Main(**vars(args)).run()
    except InvocationError as e:
        parser.error(str(e))
