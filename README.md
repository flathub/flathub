# org.freedesktop.Sdk.Extension.podman

This extension adds Podman support to Flatpak.

For example, to use Podman (`podman-remote`) with [PhpStorm](https://github.com/flathub/com.jetbrains.PhpStorm), set the following environment variable:

```bash
FLATPAK_ENABLE_SDK_EXT=podman
```

## Build

```bash
flatpak-builder --repo repo .build org.freedesktop.Sdk.Extension.podman.yml --force-clean
```
