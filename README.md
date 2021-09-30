# Flatpak for xemu

## Installation

1. [Set up Flatpak](https://www.flatpak.org/setup/)

2. Install xemu from [Flathub](https://flathub.org/apps/details/app.xemu.xemu)

`flatpak install -y app.xemu.xemu`

3. Run xemu

`flatpak run app.xemu.xemu`

To uninstall: `flatpak uninstall -y app.xemu.xemu`

## Usage

Only `~/.var/app/app.xemu.xemu/data/xemu/xemu` can be written by xemu.
The Hard Disk image has to be placed there, for example, at `~/.var/app/app.xemu.xemu/data/xemu/xemu/xbox_hdd.qcow2`.

## Build

The `flatpak-builder` package is required.

- Install the SDK

`flatpak install org.freedesktop.Platform/x86_64/21.08`

- Build xemu

`flatpak-builder --user --install --force-clean build-dir app.xemu.xemu.yml`
