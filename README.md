# Trilium Notes Flathub

## Local development

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
