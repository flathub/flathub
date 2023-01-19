# [Ruffle](https://ruffle.rs/)

ruffle is a Flash Player emulator built in the Rust programming language.

## Issues
- Currently this does not build from source, it just downloads the x86 binary. The naga package fails due to wgpu
- F: /run/user/1000/.flatpak/rs.ruffle.Ruffle/xdg-run/pulse is not a symlink to "../../flatpak/pulse" as expected: readlinkat: Invalid argument
- Immediate crash opening any swf due to `Error: Couldn't get platform clipboard`

## Building
```
flatpak install org.freedesktop.Sdk.Extension.rust-stable//22.08 org.flatpak.Builder
flatpak run org.flatpak.Builder build rs.ruffle.Ruffle.yaml --force-clean
```


## Maintenance

### cargo-sources
```
python3 ../../flatpak-builder-tools/cargo/flatpak-cargo-generator.py ./Cargo.lock -o ../cargo-sources.json
```
