#!/usr/bin/env python3
# Copyright 2018 Christoph Reiter
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

import os
import re
import sys
import argparse
import json
import yaml
from collections import OrderedDict


def json_remove_comments(json_data):

    def repl_func(match):
        comment = match.group(1).decode("utf-8")
        line = match.string[:match.start()].count(b"\n") + 1
        print("Removed comment in line {}: {}".format(line, repr(comment)),
              file=sys.stderr)
        return b""

    return re.sub(br"^\s*(/\*.*?\*/)\s*$", repl_func, json_data,
                  flags=re.MULTILINE | re.DOTALL)


def test_json_remove_comments():
    assert json_remove_comments(b"") == b""
    assert json_remove_comments(b"\"/tmp/*.txt\"") == b"\"/tmp/*.txt\""
    assert json_remove_comments(b"/*foo*/\nbar") == b"\nbar"
    assert json_remove_comments(b"  /*fo\no*/   \nbar") == b"\nbar"
    assert json_remove_comments(b"/*foo*/\nquux\n/*bar*/") == b"\nquux\n"


def json_to_yaml(json_data):
    """Takes encoded json and returns encoded yaml"""

    json_data = json_remove_comments(json_data)
    data = json.loads(json_data, object_pairs_hook=OrderedDict)

    class OrderedDumper(yaml.Dumper):

        # to get indented lists
        def increase_indent(self, flow=False, indentless=False):
            return super(OrderedDumper, self).increase_indent(flow, False)

    # to make pyyaml understand OrderedDict
    def dict_representer(dumper, data):
        return dumper.represent_dict(data.items())

    OrderedDumper.add_representer(OrderedDict, dict_representer)

    return yaml.dump(
        data, Dumper=OrderedDumper,
        default_flow_style=False, encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(
        description='Flatpak JSON to YAML converter')
    parser.add_argument('json_file', type=str,
                        help='The flatpak JSON file to convert')
    parser.add_argument('-f', '--force', dest='force',
                        default=False, action='store_true',
                        help='Overwrite existing file')
    parser.add_argument('-o', '--output', type=str, dest='out_file',
                        help='The yaml target path')
    args = parser.parse_args()

    with open(args.json_file, "rb") as h:
        out_file = args.out_file
        if out_file is None:
            out_file = os.path.splitext(args.json_file)[0] + '.yml'

        yaml_data = json_to_yaml(h.read())

        if args.force:
            flags = "wb"
        else:
            flags = "xb"

        with open(out_file, flags) as out:
            out.write(yaml_data)


if __name__ == '__main__':
    main()
