Before changing the commit number, update `cargo-sources.json` with [flatpak-cargo-generator.py](https://github.com/flatpak/flatpak-builder-tools/tree/master/cargo).
```shell
python3 ./flatpak-cargo-generator.py <project_dir>/Cargo.lock -o cargo-sources.json
```