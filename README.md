<!--
  SPDX-FileCopyrightText: 2024 Junde Yhi <junde@yhi.moe>
  SPDX-License-Identifier: CC0-1.0
-->

# Freedesktop SDK Extension - GNAT 14

This folder contains necessary files to _Flatpak_ the GNU Ada/SPARK compiler 14.x and build environment (`org.freedesktop.Sdk.Extension.gnat14`), extending the Freedesktop SDK (`org.freedesktop.Sdk`). Components included are:

- [GNAT]: the GNU [Ada] compiler
- [GNATprove]: the GNU [SPARK] analyzer
- [GPRbuild]: the GNAT Project Manager, with companion tools
- [Alire] (`alr`): the Ada Library Repository tool

[GNAT]: https://gcc.gnu.org/onlinedocs/gnat_ugn/
[Ada]: https://www.adacore.com/about-ada
[GNATprove]: https://github.com/AdaCore/spark2014
[SPARK]: https://www.adacore.com/about-spark
[GPRbuild]: https://github.com/AdaCore/gprbuild
[Alire]: https://alire.ada.dev/

## Use

Flatpak automatically mounts SDK extensions to `/usr/lib/sdk/${sdk_name}` within SDK when they're installed, but they're not automatically configured, so executables in extensions are not immediately available. If the SDK is launched manually, run the configuration script to add binaries to `$PATH`:

```sh
source /usr/lib/sdk/gnat14/enable.sh
```

If it's an application using Freedesktop SDK as its runtime (e.g. `com.vscodium.vscodium`), it likely follows the [ide-flatpak-wrapper] convention. Add an environment variable `FLATPAK_ENABLE_SDK_EXT=${sdk_names}` before launching it, where `${sdk_names}` is a comma-separated list of SDK extension names or `*` which simply turns on all available extensions. This can usually be done via Flatpak permission settings with any of `flatpak` command-line, KDE System Settings or [Flatseal].

```sh
# One-time setting
FLATPAK_ENABLE_SDK_EXT=gnat14 flatpak run com.vscodium.vscodium

# Permanent setting (need to be root)
flatpak override --env=FLATPAK_ENABLE_SDK_EXT=gnat14 com.vscodium.vscodium
```

To use this extension when building other Flatpak applications, add the ID to `sdk-extensions`, then run the enable script to add executables to `$PATH`:

```yaml
sdk: org.freedesktop.Sdk
sdk-extensions:
  - org.freedesktop.Sdk.Extension.gnat14
modules:
  - name: enable-gnat14
    buildsystem: simple
    build-commands:
      - source /usr/bin/sdk/gnat14/enable.sh
```

[Flatseal]: https://flathub.org/apps/com.github.tchx84.Flatseal
[ide-flatpak-wrapper]: https://github.com/flathub-infra/ide-flatpak-wrapper

## Build

Run `flatpak-builder` (or `org.flatpak.Builder` from Flatpak) with the manifest:

```sh
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install \
  --install-deps-from=flathub --ccache \
  --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo \
  build org.freedesktop.Sdk.Extension.gnat14.yml
```

## Limitations

So far the manifest simply downloads pre-built binaries from [alire-project/GNAT-FSF-builds](https://github.com/alire-project/GNAT-FSF-builds/releases) and [alire-project/alire](https://github.com/alire-project/alire/releases). Because there are only x86_64 builds, this extension is limited to x86_64.

## License

The metadata files included in this folder are public domain work under the CC0 1.0 license. See [CC0-1.0.txt](./LICENSES/CC0-1.0.txt) for a copy of the license text.
