### How to test the package

#### Prerequisites
- Install `flatpak` and `flatpak-builder`

#### Install steps
* clone this project
* clone the lockbook repo [here](https://github.com/lockbook/lockbook)
* generate a `cargo-sources.json` file based on lockbook repo [here](https://github.com/flatpak/flatpak-builder-tools/blob/master/cargo/README.md)
* go back to net.lockbook.Lockbook and run `flatpak-builder --force-clean --user --install --repo=repo --install builddir net.lockbook.Lockbook.json`
* run the app `flatpak run net.lockbook.Lockbook`
