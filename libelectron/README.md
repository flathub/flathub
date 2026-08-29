LibElectron Flatpak
===================

This repository contains a Flatpak build of the Arch/AUR `libelectron` package.

The Flatpak packaging is GPL-2.0-only.

The AUR `libelectron` package installs shared files under `/opt/libelectron`. Flatpak apps cannot share another app's `/app` directory, so the useful part of this repository is the reusable module at `modules/libelectron.yml`. Include that module in Flatpak application manifests that need LibElectron.

The top-level manifest is a small standalone wrapper used to build and validate the module by itself.

Build
-----

```sh
flatpak-builder --force-clean build-dir io.gitlab.linuxbombay.LibElectron.yml
```

Update Source Versions
----------------------

Flatpak manifests do not support PKGBUILD-style variable interpolation in source URLs. This repository keeps the versions and hashes in one file and generates the Flatpak module from a template.

Edit:

```text
versions/libelectron.env
```

Then regenerate:

```sh
scripts/update-libelectron-module
```

To update checksums after changing versions, run:

```sh
scripts/update-checksums
```

That downloads the configured source archives, refreshes the `*_SHA256` values in `versions/libelectron.env`, and regenerates `modules/libelectron.yml`.

That updates:

```text
modules/libelectron.yml
```

If npm dependencies change, refresh the lockfile and generated Flatpak sources:

```sh
npm install --package-lock-only --ignore-scripts --legacy-peer-deps
flatpak-node-generator npm --no-devel --node-sdk-extension org.freedesktop.Sdk.Extension.node22//25.08 -o generated-sources.json package-lock.json
```

Install locally
---------------

```sh
flatpak-builder --user --install --force-clean build-dir io.gitlab.linuxbombay.LibElectron.yml
flatpak run io.gitlab.linuxbombay.LibElectron
```

Use In Another Manifest
-----------------------

Add the module to the consuming app's `modules` list:

```yaml
modules:
  - modules/libelectron.yml
```

If the app also needs `libelectron-electron-meta`, include the sibling project's module before this one:

```yaml
modules:
  - ../libelectron-electron-meta/modules/libelectron-electron-meta.yml
  - ../libelectron/modules/libelectron.yml
```

For shared dependency behavior, applications should use `io.gitlab.linuxbombay.LibElectron.BaseApp//2026.5` instead of including the modules directly.

The module installs:

```text
/app/opt/libelectron
/app/opt/libelectron/node_modules
/app/opt/libelectron/libsplash
/app/opt/libelectron/libadblock
/app/opt/libelectron/libuseragent
/app/opt/libelectron/electron -> /app/bin/libelectronmeta
```

The consuming app is still responsible for providing `/app/bin/libelectronmeta`. Use the sibling `../libelectron-electron-meta` project for that module.
