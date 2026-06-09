# org.freedesktop.Sdk.Extension.podman

This extension adds Podman support for Flatpak applications.

To use the Podman SDK, set the following environment variable per application:

```bash
FLATPAK_ENABLE_SDK_EXT=podman
```

For applications that require Podman socket support:

```bash
systemctl --user enable podman.socket --now
flatpak override --user --filesystem=xdg-run/podman:ro app-id
```

The socket path should be available inside the Flatpak application:

```bash
$XDG_RUNTIME_DIR/podman/podman.sock
```

## Usage

### PhpStorm

To use with [PhpStorm](https://github.com/flathub/com.jetbrains.PhpStorm), make sure to set the connection type to 'Podman'.

You may also need to set the podman socket path to allow full container integration (`$XDG_RUNTIME_DIR/podman/podman.sock`).

### Visual Studio Code / VSCodium

To use with [VSCode](https://github.com/flathub/com.visualstudio.code), allow access to the Podman socket:

```bash
flatpak override --user --filesystem=xdg-run/podman:ro com.visualstudio.code
```

Open VSCode, run command `Open User Settings (JSON)` and append:

```json
"containers.composeCommand": "/usr/lib/sdk/podman/bin/podman-compose",
"containers.containerCommand": "/usr/lib/sdk/podman/bin/podman-remote",
"dev.containers.dockerComposePath": "/usr/lib/sdk/podman/bin/podman-compose",
"dev.containers.dockerPath": "/usr/lib/sdk/podman/bin/podman-remote",
"dev.containers.dockerSocketPath": "/run/user/<UID>/podman/podman.sock",
"docker.dockerPath": "/usr/lib/sdk/podman/bin/podman-remote"
```

> Note: Replace <UID> with the user-id that runs the socket.

Restart the editor to apply changes.

### Devcontainers

Update the project `devcontainer.json` file with `runArgs` that apply to Podman:

```json
{
  "runArgs": ["--userns=keep-id", "--init"]
}
```

One may also want to append `--network=systemd-networkname` to allow network communication, and `--security-opt=label=disable` to prevent SELinux from setting filesystem labels.

It may be required for certain devcontainer images to force the Docker format when building:

```json
{
  "runArgs": ["--userns=keep-id", "--init"],
  "build": {
    "options": ["--format=docker"]
  }
}
```

## Build

```bash
flatpak-builder --repo repo .build org.freedesktop.Sdk.Extension.podman.yml --force-clean
```
