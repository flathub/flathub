# [Ruffle](https://ruffle.rs/)

ruffle is a Flash Player emulator built in the Rust programming language.

## Issues

- Currently this does not build from source, it just downloads the x86 binary. The naga package fails due to wgpu.
- Hard depencnecy on x11.


## Building
```
flatpak run org.flatpak.Builder build rs.ruffle.Ruffle.yaml --force-clean
```
