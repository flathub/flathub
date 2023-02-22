#!/usr/bin/env python3

__license__ = 'MIT'

from pathlib import Path

import base64
import binascii
import json
import subprocess
import tempfile

# The parent path of the extracted Godot version to use. CHANGE FOR UPDATES
godot_path = './godot-4.0-rc3'

# The projects to generate the sources from. CHANGE IF NECESSARY
projects = [
    'modules/mono/glue/GodotSharp/GodotSharp/GodotSharp.csproj',
    'modules/mono/glue/GodotSharp/Godot.SourceGenerators.Internal/Godot.SourceGenerators.Internal.csproj',
    'modules/mono/editor/GodotTools/GodotTools/GodotTools.csproj',
    'modules/mono/editor/GodotTools/GodotTools.BuildLogger/GodotTools.BuildLogger.csproj',
    'modules/mono/editor/GodotTools/GodotTools.IdeMessaging/GodotTools.IdeMessaging.csproj',
    'modules/mono/editor/GodotTools/GodotTools.IdeMessaging.CLI/GodotTools.IdeMessaging.CLI.csproj',
    'modules/mono/editor/GodotTools/GodotTools.OpenVisualStudio/GodotTools.OpenVisualStudio.csproj',
    'modules/mono/editor/GodotTools/GodotTools.ProjectEditor/GodotTools.ProjectEditor.csproj',
    'modules/mono/editor/Godot.NET.Sdk/Godot.NET.Sdk/Godot.NET.Sdk.csproj',
    'modules/mono/editor/Godot.NET.Sdk/Godot.SourceGenerators/Godot.SourceGenerators.csproj'
]

def main():
    sources = []

    # Create a temporary directory to store all of the NuGet packages in
    with tempfile.TemporaryDirectory(dir=Path()) as tmp:
        # Iterate over all of the projects that needs to be restored
        for project in projects:
            # Restore the project
            subprocess.run([
                'flatpak', 'run',
                '--env=DOTNET_CLI_TELEMETRY_OPTOUT=true',
                '--env=DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true',
                '--command=sh', '--runtime=org.freedesktop.Sdk//22.08', '--share=network',
                '--filesystem=host', 'org.freedesktop.Sdk.Extension.dotnet7//22.08', '-c',
                'PATH="${PATH}:/usr/lib/sdk/dotnet7/bin" LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/lib/sdk/dotnet7/lib" exec dotnet restore "$@"',
                '--', '--packages', tmp, godot_path+'/'+project])

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
