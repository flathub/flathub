#!/usr/bin/env python3

__license__ = 'MIT'

from pathlib import Path

import argparse
import base64
import binascii
import json
import subprocess
import tempfile
import concurrent.futures

def main():
    # Bump this to latest freedesktop runtime version.
    freedesktop_default = '24.08'
    # Bump this to an LTS dotnet version.
    dotnet_default = '8'

    parser = argparse.ArgumentParser()
    parser.add_argument('output', help='The output JSON sources file')
    parser.add_argument('project', nargs='+', help='The project file(s)')
    parser.add_argument('--runtime', '-r', nargs='+', default=[None], help='The target runtime(s) to restore packages for')
    parser.add_argument('--freedesktop', '-f', help='The target version of the freedesktop sdk to use', 
                        default=freedesktop_default)
    parser.add_argument('--dotnet', '-d', help='The target version of dotnet to use', 
                        default=dotnet_default)
    parser.add_argument('--destdir',
                        help='The directory the generated sources file will save sources to',
                        default='nuget-sources')
    parser.add_argument('--dotnet-args', '-a', nargs=argparse.REMAINDER, 
                        help='Additional arguments to pass to the dotnet command')
    args = parser.parse_args()

    sources = []

    for path in Path("packages").glob('**/*.nupkg.sha512'):
        name = path.parent.parent.name
        version = path.parent.name
        filename = '{}.{}.nupkg'.format(name, version)
        url = 'https://api.nuget.org/v3-flatcontainer/{}/{}/{}'.format(name, version,
                                                                        filename)

        destpath = f"{args.destdir}" #/{name}/{version}"
        # destpath.mkdir(exist_ok=True)
        with path.open() as fp:
            sha512 = binascii.hexlify(base64.b64decode(fp.read())).decode('ascii')

        sources.append({
            'type': 'file',
            # 'archive-type': 'zip',
            'url': url,
            'sha512': sha512,
            'dest': destpath,
            'dest-filename': filename,
            # 'strip-components': '0'
        })

    with open(args.output, 'w') as fp:
        json.dump(
            sorted(sources, key=lambda n: n.get("dest-filename")),
            fp,
            indent=4
        )

if __name__ == '__main__':
    main()
