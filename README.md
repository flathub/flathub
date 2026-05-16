# org.freedesktop.Sdk.Extension.podman

This extension adds Podman support for Flatpak applications.

To use the Podman SDK, set the following environment variable per application:

```bash
FLATPAK_ENABLE_SDK_EXT=podman
```

For applications that require Podman socket support:

```bash
systemctl --user enable podman.socket --now
flatpak override --user --filesystem=xdg-run/podman:ro com.visualstudio.code
```

## Usage

### PhpStorm

To use with [PhpStorm](https://github.com/flathub/com.jetbrains.PhpStorm), make sure to set the connection type to 'Podman'.

### Visual Studio Code

To use with [VSCode](https://github.com/flathub/com.visualstudio.code), run command `Open User Settings (JSON)` and append:

```json
"dev.containers.dockerPath": "/usr/lib/sdk/podman/bin/podman-remote",
"dev.containers.dockerSocketPath": "/run/user/1000/podman/podman.sock",
"docker.dockerPath": "/usr/lib/sdk/podman/bin/podman-remote"
```

Restart the editor to apply changes.

## Build

```bash
flatpak-builder --repo repo .build org.freedesktop.Sdk.Extension.podman.yml --force-clean
```
