# Trilium Notes Flathub

Trilium Notes is a free and open-source, cross-platform hierarchical note taking application with focus on building large personal knowledge bases.

## 🐞 Reporting issues

This repository is the official Flathub build recipe for Trilium Notes. To report issues related to the Trilium Notes application itself, please refer to [Trilium Notes issue tracker](https://github.com/TriliumNext/Trilium/issues).

## 📦 Package architecture & design

* This Flathub package is built from source, with `generated-sources.json` being built by a script the Trilium repository side and stored as part of this repo. Whereas the main build system uses Electron Forge, the Flathub package is built from scratch.
* As a hardening of the Electron process, we flip some Electron fuses (e.g. `RunAsNode`, `EnableNodeOptionsEnvironmentVariable` are disabled).
* Build info is stamped from the commit date, to make the build process idempotent.
* The package manager is pnpm and the sources are generated via `flatpak-node-generator` with a post-processing step of removing the Playwright sources which are not needed during the offline build process.
* The official Trilium packages (.deb, .rpm, even .flatpak which we ship separately) are packaged with ASAR. For the Flathub build we've decided to go with plain files instead, for simplifying the build process and also because there is no actual benefit (tamper-sealing is already handled by the content-addressed OSTree deployment). The only downside is that the package is 11 MB bigger (probably due to the file headers).
* The data directory lives in the app sandbox (`~/.var/app/org.triliumnotes.Trilium/data/trilium-data`), set via `TRILIUM_DATA_DIR` in the launch wrapper, so no filesystem permission is needed for it. A `TRILIUM_DATA_DIR` set through `flatpak override`/Flatseal takes precedence, and existing host data at `~/.local/share/trilium-data` is used if there is a user override.
* Read-only access to the common XDG directories (downloads, documents, pictures, videos) is requested because dragging a file into the note tree cannot be read ([electron#30650](https://github.com/electron/electron/issues/30650)); without the grant, drag & drop import fails.
* There is currently no automatic update of the Flatpak manifest in place, but it is planned for the near future.

## 🖥️ Local development

First, ensure Flatpak builder is installed:

```sh
flatpak install flathub org.flatpak.Builder
```

### Build and install locally

```sh
flatpak run org.flatpak.Builder --user --install --force-clean \
    builddir org.triliumnotes.Trilium.yml
```

### Lint the manifest

```sh
flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
    manifest org.triliumnotes.Trilium.yml
```

### Lint the built repo

```sh
flatpak run org.flatpak.Builder --user --force-clean --default-branch=test \
    --repo=repo builddir org.triliumnotes.Trilium.yml
flatpak run --env=REPO=https://github.com/flathub/org.triliumnotes.Trilium \
    --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

### Run locally

* Against your data dir: `flatpak run org.triliumnotes.Trilium`
* With a temporary data dir: `flatpak run --env=TRILIUM_DATA_DIR=/tmp/trilium-flatpak-test org.triliumnotes.Trilium`
