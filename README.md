Electrum flatpak buildfile
==========================

1. Install dependencies with `flatpak install org.kde.Sdk//5.12 org.kde.Platform//5.12
2. Build it with `flatpak-builder build-dir org.electrum.electrum.json`. Sometiems you have to install `eu-strip` command before it can compile.
3. Run the test build with `flatpak-builder --run build-dir org.electrum.electrum.json electrum`
4. Install the test build following [this description.](http://docs.flatpak.org/en/latest/first-build.html#install-the-app).
