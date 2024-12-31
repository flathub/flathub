# org.freedesktop.Sdk.Extension.rustup-stable-minimal

This freedesktop SDK extension contains a rustup installation from the `stable` channel using the `minimal` profile. Rustup with profile minimal includes cargo and the rustc compiler.

This extension exists for those use cases were a rustup installation is needed at build time.
Tools like [Cargokit](https://github.com/irondash/cargokit) and [flutter_rust_bridge](https://github.com/fzyzcjy/flutter_rust_bridge) use the Rust tools via rustup and fail to build without it.

For use cases that do not depend on rustup there is also the [org.freedesktop.Sdk.Extension.rust-stable
](https://github.com/flathub/org.freedesktop.Sdk.Extension.rust-stable) SDK extension. It has an [explicit statement](https://github.com/flathub/org.freedesktop.Sdk.Extension.rust-stable/issues/4) that rustup is out of its scope.

The read-only nature of SDK extensions limits the rustup features that can be used at application build time. The stable toolchain for the architecture is included, but there is no possibility to perform a `rustup toolchain install`. Additionally, updates have to be done by updating the extension, a `rustup self update` will fail.

## Automation
Tooling could be created to generate `rust-stable-minimal.json` from [channel-rust-stable.toml](https://static.rust-lang.org/dist/channel-rust-stable.toml). This is the approach already taken by [flatpak-cargo-generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/cargo) for processing Cargo.lock. With this and GitHub Actions creating extension updates could be automated, but this hasn't been done yet.
