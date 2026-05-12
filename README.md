# Raffi Flatpak

This directory contains Raffi's Flatpak packaging.

`al.raffi.raffi.yml` is the source-built Flathub submission manifest. It declares the app source, Bun/npm dependencies, Go modules, Electron downloads, Bun, and ffmpeg as Flatpak sources so the build can run without network access.

`al.raffi.raffi.local.yml` is only for local smoke testing from `release/linux-unpacked`.

```sh
cd raffi-desktop
flatpak/build-local.sh
flatpak run al.raffi.raffi
```

Regenerate dependency source manifests after changing `bun.lock`, `go.mod`, or `go.sum`:

```sh
cd raffi-desktop
flatpak/update-sources.sh
```

Before submitting to Flathub, validate from a checkout containing these files:

```sh
cd raffi-desktop/flatpak
flatpak run --command=flathub-build org.flatpak.Builder --install al.raffi.raffi.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest al.raffi.raffi.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream al.raffi.raffi.metainfo.xml
desktop-file-validate al.raffi.raffi.desktop
```

For future releases:

1. Update the app version and AppStream release entry.
2. Update screenshot URLs if they point at a versioned tag.
3. Regenerate `bun-sources.json` and `go-sources.json` if `bun.lock`, `go.mod`, or `go.sum` changed.
4. Update `tag` in `al.raffi.raffi.yml`.
5. After the release tag is pushed, pin the Flathub submission manifest to that tag's commit hash.
