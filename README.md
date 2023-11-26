# TrguiNG
[TrguiNG](https://github.com/openscopeproject/TrguiNG) is made with Tauri and built using npm and cargo packages.

## Source generation
To transform the npm/cargo package locks into flatpak sources, [flatpak-builder-tools](https://github.com/flatpak/flatpak-builder-tools) is used.

```sh
# Generate TrguiNG sources for use with cargo
<path-to flatpak-builder-tools>/cargo/flatpak-cargo-generator.py -o cargo-sources.json <path-to trguing>/src-tauri/Cargo.lock

# Generate TrguiNG sources for use with npm
flatpak-node-generator --no-requests-cache -r -o node-sources.json npm <path-to trguing>/package-lock.json
```