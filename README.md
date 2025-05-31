# Flatpak manifest for [Stretch Break](https://github.com/pieterdd/StretchBreak/)

### Build instructions
- In case of Rust dependency changes, get [flatpak-cargo-generator](https://pypi.org/project/flatpak-cargo-generator/), then execute `flatpak-cargo-generator /path/to/code/repo/Cargo.lock -o /path/to/manifest/repo/cargo-sources.json`.
- Run `./flatpak-build.sh` to do a local test build. This script is not used by Flathub's build system.

Note that the GNOME Shell extension will have to be installed separately.
