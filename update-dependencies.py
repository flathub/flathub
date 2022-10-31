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
    'https://oss.sonatype.org/content/repositories/snapshots/',
]


def parse_url(url):
    for base in REPO_BASES:
        if url.startswith(base):
            relpath = url[len(base):]
            return RepoURL(url=url, relpath=relpath, base=base)
    else:
        raise ValueError(f'Repository base of {url} unknown')


def parse_output(directory):
    output = directory / 'maven-output'
    with output.open() as source:
        for line in source:
            match = DOWNLOAD_PATTERN.search(line)
            if not match:
                continue
            url = parse_url(match[1])
            destdir = str((Path('.m2/repository') / url.relpath).parent)
            destname = Path(url.relpath).name
            if destname == 'maven-metadata.xml':
                candidates = list((directory / url.relpath).parent.glob('maven-metadata*.xml'))
                destname = candidates[0].name
            yield {
                'type': 'file',
                'url': url.url,
                'dest': destdir,
                'dest-filename': destname,
                'sha512': hashlib.sha512(((directory / url.relpath).parent / destname).read_bytes()).hexdigest()
            }


def main():
    directory = Path(sys.argv[1])
    files = sorted(parse_output(directory), key=lambda f: f['url'])

    with open('maven-dependencies.json', 'w') as sink:
        json.dump(files, sink, indent=2)


if __name__ == '__main__':
    main()
