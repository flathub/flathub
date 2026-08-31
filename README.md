## Local development

To build and run locally (against your data dir):

```sh
flatpak-builder --user --install --force-clean builddir org.triliumnotes.Trilium.yml
flatpak run org.triliumnotes.Trilium
```

To run with a temporary data dir:

```sh
flatpak run --env=TRILIUM_DATA_DIR=/tmp/trilium-flatpak-test org.triliumnotes.Trilium
```