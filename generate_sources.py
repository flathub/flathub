#!/usr/bin/env python3

__license__ = 'MIT'

from pathlib import Path

import base64
import binascii
import json
import subprocess
import tempfile
import glob
import os

def main():
    sources = []

    # Find all projects to restore for any extracted godot source
    projects = glob.glob('./godot-*/**/*.csproj', recursive=True)

    # Create a temporary directory to store all of the NuGet packages in
    with tempfile.TemporaryDirectory(dir=Path()) as tmp:
        # Iterate over all of the projects that needs to be restored
        for project in projects:
            # Restore the project
            subprocess.run([
                'flatpak', 'run',
                '--env=DOTNET_NOLOGO=true',
                '--env=DOTNET_CLI_TELEMETRY_OPTOUT=true',
                '--env=DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true',
                '--command=sh', '--runtime=org.freedesktop.Sdk//22.08', '--share=network',
                '--filesystem=%s' % os.getcwd(), 'org.freedesktop.Sdk.Extension.dotnet7//22.08', '-c',
                'PATH="${PATH}:/usr/lib/sdk/dotnet7/bin" LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/lib/sdk/dotnet7/lib" exec dotnet restore "$@"',
                '--', '--packages', tmp, project])

            # Append the package data to sources
            for path in Path(tmp).glob('**/*.nupkg.sha512'):
                name = path.parent.parent.name
                version = path.parent.name
                filename = '{}.{}.nupkg'.format(name, version)
                url = 'https://api.nuget.org/v3-flatcontainer/{}/{}/{}'.format(name, version, filename)
                with path.open() as fp:
                    sha512 = binascii.hexlify(base64.b64decode(fp.read())).decode('ascii')
                    sources.append({
                        'type': 'file',
                        'url': url,
                        'sha512': sha512,
                        'dest': "nuget-sources",
                        'dest-filename': filename,
                    })

    # Save the sources into a JSON file
    with open('nuget/nuget-sources.json', 'w') as fp:
        json.dump(
            sorted(sources, key=lambda n: n.get("dest-filename")),
            fp,
            indent=4
        )


if __name__ == '__main__':
    main()
