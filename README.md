# Puzzled - Flathub

Manifest is based on:

- Upstream Release manifest
- https://github.com/flatpak/flatpak-builder-tools/tree/master/cargo#cargo_home-is-set-by-buildsystem

## Update

1. Update `cargo-sources.json` using https://github.com/flatpak/flatpak-builder-tools/tree/master/cargo
2. Mirror changes to the manifest from upstream, if any.
3. Update tag to pull sources from.
4. Test build locally

```bash
flatpak run --command=flathub-build org.flatpak.Builder --repo=repo --disable-rofiles-fuse --install-deps-from=flathub --force-clean de.til7701.Puzzled.json
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest de.til7701.Puzzled.json
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream .flatpak-builder/build/puzzled/data/de.til7701.Puzzled.metainfo.xml.in
```
