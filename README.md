## Local development

To build locally:

```sh
flatpak-builder --user --force-clean builddir org.triliumnotes.Trilium.yml
```

To run locally:

```sh
flatpak-builder --run builddir org.triliumnotes.Trilium.yml trilium
```