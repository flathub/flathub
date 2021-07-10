# Flatpak Go Get Generator
Tool to automatically create the source list for a Go module.

The script does not require Go in the host system.

## Usage
1. In the manifest, give the Go module network access and set GOPATH to $PWD.

  Example:
```json
{
  "name": "writeas-cli",
  "buildsystem": "simple",
  "build-options": {
    "env": {
      "GOBIN": "/app/bin/"
    },
    "build-args": [
      "--share=network"
    ]
  },
  "build-commands": [
    ". /usr/lib/sdk/golang/enable.sh; export GOPATH=$PWD; go get github.com/writeas/writeas-cli/cmd/writeas"
  ]
}
```

2. Run flatpak-builder with `--keep-build-dirs`.
3. Run `go-get/flatpak-go-get-generator.py <build-dir>` with build-dir pointing the the build directory in `.flatpak-builder/build`.
4. Convert the source list to YAML if necessary.
5. Add the list to the sources field of the Go module in the manifest.
6. Change build command from `go get` to `go install`.
7. Remove network access.

**The script assumes the networked built was run with `GOPATH=$PWD`.**

## Example final module
```json
{
      "name": "writeas-cli",
      "buildsystem": "simple",
      "build-options": {
        "env": {
          "GOBIN": "/app/bin/"
        }
      },
      "build-commands": [
        ". /usr/lib/sdk/golang/enable.sh; export GOPATH=$PWD; go install github.com/writeas/writeas-cli/cmd/writeas"
      ],
      "sources": [
        {
          "type": "git",
          "url": "https://github.com/atotto/clipboard",
          "commit": "aa9549103943c05f3e8951009cdb6a0bec2c8949",
          "dest": "src/github.com/atotto/clipboard"
        },
        ...
      ]
    }
```

