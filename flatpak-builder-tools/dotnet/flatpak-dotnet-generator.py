#!/usr/bin/env python3

__license__ = 'MIT'

from pathlib import Path

import argparse
import base64
import binascii
import json
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output', help='The output JSON sources file')
    parser.add_argument('project', help='The project file')
    parser.add_argument('--runtime', '-r', help='The target runtime to restore packages for')
    parser.add_argument('--destdir',
                        help='The directory the generated sources file will save sources to',
                        default='nuget-sources')
    args = parser.parse_args()

    sources = []

    with tempfile.TemporaryDirectory(dir=Path()) as tmp:
        runtime_args = []
        if args.runtime:
            runtime_args.extend(('-r', args.runtime))

        subprocess.run(['flatpak', 'run', '--command=sh',
                        '--runtime=org.freedesktop.Sdk//18.08', '--share=network',
                        '--filesystem=host',
                        'org.freedesktop.Sdk.Extension.dotnet//18.08', '-c',
                        '. /usr/lib/sdk/dotnet/enable.sh; exec dotnet restore "$@"', '--',
                        '--packages', tmp, args.project] + runtime_args)

        for path in Path(tmp).glob('**/*.nupkg.sha512'):
            name = path.parent.parent.name
            version = path.parent.name
            filename = '{}.{}.nupkg'.format(name, version)
            url = 'https://api.nuget.org/v3-flatcontainer/{}/{}/{}'.format(name, version,
                                                                           filename)

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
        json.dump(sources, fp, indent=4)


if __name__ == '__main__':
    main()
