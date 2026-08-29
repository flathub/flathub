LibElectron Electron Meta Flatpak
=================================

This repository contains a Flatpak adaptation of the Arch/AUR `libelectron-electron-meta` package.

The module installs a controlled Electron release and a `/app/bin/libelectronmeta` wrapper. It currently uses Electron 42.10.1.

The Flatpak build downloads the official Electron Linux release ZIPs as declared sources so the build itself does not need network access.

Build
-----

```sh
flatpak-builder --force-clean build-dir io.gitlab.linuxbombay.LibElectronElectronMeta.yml
```

Install locally
---------------

```sh
flatpak-builder --user --install --force-clean build-dir io.gitlab.linuxbombay.LibElectronElectronMeta.yml
flatpak run io.gitlab.linuxbombay.LibElectronElectronMeta
```

Use In Another Manifest
-----------------------

Flatpak apps cannot use files from another app's `/app` directory. A consuming application must include this reusable module in its own manifest instead of depending on the installed validation Flatpak.

Example when this project sits beside `libelectron` under `/mnt/Storage/Flatpak`:

```yaml
modules:
  - ../libelectron-electron-meta/modules/libelectron-electron-meta.yml
  - ../libelectron/modules/libelectron.yml
  - name: your-app
```

The consuming manifest should include `org.freedesktop.Sdk.Extension.node20` in `sdk-extensions` unless the module is adapted to use a different Node SDK.

Versioning
----------

To change the controlled Electron version, edit:

```text
versions/electron.env
```

Example:

```text
ELECTRON_MAJOR_VERSION=42
ELECTRON_VERSION=42.10.1
ELECTRON_X64_SHA256=2452b27112d92387471fa2488aafac85d79ea3f2ee1216c0abd5150d6c12362b
ELECTRON_ARM64_SHA256=20e68d6c4e47f3ebf59de7c6b1f8b8bec6a6ebda6a451132f9b465f3f13ce467
```

Then regenerate:

```sh
scripts/update-electron-meta-module
```

That updates these generated files:

```text
modules/libelectron-electron-meta.yml
scripts/libelectronmeta
```
