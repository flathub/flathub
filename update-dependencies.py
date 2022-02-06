#!/usr/bin/env python3
# Copyright Sebastian Wiesner <sebastian@swsnr.de>
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

import sys
import re
import json
import hashlib
from pathlib import Path
from collections import namedtuple

DOWNLOAD_PATTERN = re.compile("""Downloaded from .*: (https?://[^ ]+)""")


RepoURL = namedtuple('RepoURL', 'url relpath base')


REPO_BASES = [
    'https://repo.maven.apache.org/maven2/',
]


def parse_url(url):
    for base in REPO_BASES:
        if url.startswith(base):
            relpath = url[len(base):]
            return RepoURL(url=url, relpath=relpath, base=base)
    else:
        raise ValueError(f'Repository base of {url} unknown')


def main():
    directory = Path(sys.argv[1])
    output = directory / 'maven-output'
    with output.open() as source:
        matches = (DOWNLOAD_PATTERN.search(line) for line in source)
        urls = (parse_url(m[1]) for m in matches if m)

        files = [
            {
                'type': 'file',
                'url': url.url,
                'dest': str((Path('.m2/repository') / url.relpath).parent),
                'dest-filename': Path(url.relpath).name,
                'sha512': hashlib.sha512((directory / url.relpath).read_bytes()).hexdigest()
            }
            for url in urls
        ]
        files.sort(key=lambda f: f['url'])

    with open('maven-dependencies.json', 'w') as sink:
        json.dump(files, sink, indent=2)


if __name__ == '__main__':
    main()
