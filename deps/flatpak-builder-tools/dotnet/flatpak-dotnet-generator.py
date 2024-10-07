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
    # Bump this to latest freedesktop runtime version.
    freedesktop_default = '22.08'
    # Bump this to an LTS dotnet version.
    dotnet_default = '6'

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

        subprocess.run([
            'flatpak', 'run',
            '--env=DOTNET_CLI_TELEMETRY_OPTOUT=true',
            '--env=DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true',
            '--command=sh', f'--runtime=org.freedesktop.Sdk//{args.freedesktop}', '--share=network',
            '--filesystem=host', f'org.freedesktop.Sdk.Extension.dotnet{args.dotnet}//{args.freedesktop}', '-c',
            f'PATH="${{PATH}}:/usr/lib/sdk/dotnet{args.dotnet}/bin" LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/lib/sdk/dotnet{args.dotnet}/lib" exec dotnet restore "$@"',
            '--', '--packages', tmp, args.project] + runtime_args)

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
        json.dump(
            sorted(sources, key=lambda n: n.get("dest-filename")),
            fp,
            indent=4
        )


if __name__ == '__main__':
    main()
