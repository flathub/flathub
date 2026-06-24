# flatpak-spm-generator

Tool to automatically generate `flatpak-builder` manifest JSON from a Swift package.

## Requirements

Swift is required to execute the script. It is not only written as a Swift script, but also uses Swift to get information about the dependencies of the package.

## Usage

The first step is to convert the dependencies declared in the `Package.swift` file as well as the dependencies of those dependencies into a format flatpak-builder can understand,
and generate a script that makes SPM use the offline dependencies.
```
swift flatpak-spm-generator.swift ./quickstart ./quickstart
```

The first path is the directory containing the Swift package, and the second path is the directory containing the Flatpak manifest file.

The tool creates a `generated-sources.json` and a `setup-offline.sh` file in the directory of the Flatpak manifest. Here is a sample module of a manifest showing how to use those generated files.
```json
{
    "name": "quickstart",
    "buildsystem": "simple",
    "sources": [
        {
            "type": "dir",
            "path": "."
        },
        "generated-sources.json"
    ],
    "build-commands": [
        "./setup-offline.sh",
        "swift build -c release --static-swift-stdlib --skip-update",
        "install -Dm755 .build/release/quickstart /app/bin/quickstart"
    ]
}
```

See the quickstart project for a complete example.

### Suffix

Optionally, add a third argument with a suffix for the file names. This can be helpful if multiple Swift packages are present.
```
swift flatpak-spm-generator.swift ./quickstart ./quickstart quickstart
```

The tool then creates a `generated-sources-<suffix>.json` and a `setup-offline-<suffix>.sh` file in the directory of the Flatpak manifest.
