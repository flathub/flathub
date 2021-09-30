# Flatpak for Xemu

## Installation

1. [Set up Flatpak](https://www.flatpak.org/setup/)

2. Install Xemu from [Flathub](https://flathub.org/apps/details/app.xemu.Xemu)

`flatpak install -y app.xemu.Xemu`

3. Run Xemu

`flatpak run app.xemu.Xemu`

To uninstall: `flatpak uninstall -y app.xemu.Xemu`

## Usage

Only `~/.var/app/app.xemu.Xemu/data/xemu/xemu` can be written by Xemu.
The Hard Disk image has to be placed there, for example, at `~/.var/app/app.xemu.Xemu/data/xemu/xemu/xbox_hdd.qcow2`

## Build

The `flatpak-builder` package is required.

- Install the SDK

`flatpak install org.freedesktop.Platform/x86_64/21.08`

- Build Xemu

`flatpak-builder --user --install --force-clean build-dir app.xemu.Xemu.yml`
