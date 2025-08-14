#!/usr/bin/env python3

__license__ = 'MIT'

from pathlib import Path

import argparse
import base64
import binascii
import json
import requests
import subprocess
import tempfile


def main():
    # Bump this to latest freedesktop runtime version.
    freedesktop_default = '24.08'
    # Bump this to an LTS dotnet version.
    dotnet_default = '8'

    parser = argparse.ArgumentParser()
    parser.add_argument('output', help='The output JSON sources file')
    parser.add_argument('project', help='The project file')
    parser.add_argument('--runtime', '-r', help='The target runtime to restore packages for')
    parser.add_argument('--freedesktop', '-f', help='The target version of the freedesktop sdk to use',
                        default=freedesktop_default)
    parser.add_argument('--dotnet', '-d', help='The target version of dotnet to use',
                        default=dotnet_default)
    parser.add_argument('--destdir',
                        help='The directory the generated sources file will save sources to',
                        default='nuget-sources')
    args = parser.parse_args()

    sources = []

    with tempfile.TemporaryDirectory(dir=Path()) as tmp:
        runtime_args = []
        if args.runtime:
            runtime_args.extend(('-r', args.runtime))

        subprocess.run(['dotnet', 'restore', '--packages', tmp, args.project] + runtime_args)

        for path in Path(tmp).glob('**/*.nupkg.sha512'):
            name = path.parent.parent.name
            version = path.parent.name
            filename = '{}.{}.nupkg'.format(name, version)
            url = 'https://api.nuget.org/v3-flatcontainer/{}/{}/{}'.format(name, version,
                                                                           filename)
            if requests.head(url).status_code != 200:
                if 'avalonia' in name:
                    url = 'https://nuget-feed-nightly.avaloniaui.net/v3/package/{}/{}/{}'.format(name, version,filename)
                else:
                    url = 'https://pkgs.dev.azure.com/jonko0493/haroohie-public/_apis/packaging/feeds/haroohie/nuget/packages/{}/versions/{}/content?api-version=7.1-preview.1'.format(name, version)

            with path.open() as fp:
                sha512 = binascii.hexlify(base64.b64decode(fp.read())).decode('ascii')

            sources.append({
                'type': 'file',
                'url': url,
                'sha512': sha512,
                'dest': args.destdir,
                'dest-filename': filename,
            })

    with open(args.output, 'w') as fp:
        json.dump(
            sorted(sources, key=lambda n: n.get("dest-filename")),
            fp,
            indent=4
        )


if __name__ == '__main__':
    main()
