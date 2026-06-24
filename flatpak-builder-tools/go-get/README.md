# Flatpak Go Get Generator
Tool to automatically create the source list for a Go module (legacy).

It runs the build in a legacy `GOPATH` mode.
For a module-aware mode, see [go-modules](../go-modules/README.md) or [flatpak-go-vendor-generator](./flatpak-go-vendor-generator.py) script.

The script does not require Go in the host system.

## Usage
1. In the manifest, give the Go module network access and set GOPATH to $PWD.

  Example manifest module (json):
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

  Example manifest (yaml):
```yaml
app-id: writeas-cli
runtime: org.freedesktop.Platform
runtime-version: '21.08'
sdk: org.freedesktop.Sdk
sdk-extensions:
  - org.freedesktop.Sdk.Extension.golang
command: echo "Done"
modules:
  - name: writeas
    buildsystem: simple
    build-options:
      append-path: /usr/lib/sdk/golang/bin
      env:
        GOBIN: /app/bin
        GO111MODULE: off
        GOPATH: /run/build/writeas
      build-args:
        - --share=network
    build-commands:
      - go get github.com/writeas/writeas-cli/cmd/writeas
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

