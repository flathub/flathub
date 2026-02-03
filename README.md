### How to test the package

#### Prerequisites
- Install `flatpak` and `flatpak-builder`
- Install `org.freedesktop.Sdk` extenstion `flatpak install org.freedesktop.Platform/x86_64/23.08` select `user`
- Install `org.freedesktop.Sdk.Extension.rust-stable` `flatpak install flathub org.freedesktop.Sdk.Extension.rust-stable/x86-64/25.08` select `user`

#### Install steps
* clone this project
* clone the lockbook repo [here](https://github.com/lockbook/lockbook)
* generate a `cargo-sources.json` file based on lockbook repo [here](https://github.com/flatpak/flatpak-builder-tools/blob/master/cargo/README.md)
* go back to net.lockbook.Lockbook and run `flatpak-builder --force-clean --user --install --repo=repo --install builddir net.lockbook.Lockbook.json`
* run the app `flatpak run net.lockbook.Lockbook`
