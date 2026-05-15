# org.freedesktop.Sdk.Extension.podman

This extension adds Podman support to Flatpak.

For example, to opt-in for Podman (`podman-remote`) support, set the following environment variable:

```bash
FLATPAK_ENABLE_SDK_EXT=podman
```

## Usage

### PhpStorm

To use with [PhpStorm](https://github.com/flathub/com.jetbrains.PhpStorm), make sure to set the connection type to 'Podman'.

### Visual Studio Code

To use with [VSCode](https://github.com/flathub/com.visualstudio.code), run command `Open User Settings (JSON)` and append:

```json
"dev.containers.dockerPath": "/usr/lib/sdk/podman/bin/podman-remote",
"docker.dockerPath": "/usr/lib/sdk/podman/bin/podman-remote"
```

## Build

```bash
flatpak-builder --repo repo .build org.freedesktop.Sdk.Extension.podman.yml --force-clean
```
