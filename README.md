# io.github.moonlight_mod.moonlight-installer

Flatpak package for [moonlight installer](https://github.com/moonlight-mod/moonlight-installer)

## TODO

- [x] .desktop file
- [x] correct versioning

## Building

```sh
# install flatpak-builder
flatpak install --user org.flatpak.Builder

# build and install
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo builddir io.github.moonlight_mod.moonlight-installer.yml

# build single-file bundle (optional)
flatpak build-bundle repo moonlight-installer.flatpak io.github.moonlight_mod.moonlight-installer --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
```

## Generating sources

```sh
# get flatpak-builder-tools
git submodule update --init --recursive

# generate sources
flatpak-builder-tools/cargo/flatpak-cargo-generator.py ../path/to/moonlight-mod/Cargo.lock -o cargo-sources.json
```

## Authoring a new release

1. [Generate sources](#generating-sources)
2. Change commit ID in the YML file
3. Add release information `<release/>` to the XML file
