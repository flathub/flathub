# Generate rust sources

```
git clone https://github.com/wiiznokes/fan-control.git --branch flatpak
git clone https://github.com/flatpak/flatpak-builder-tools
# pip install aiohttp
python3 flatpak-builder-tools/cargo/flatpak-cargo-generator.py fan-control/Cargo.lock -o cargo-sources.json
```

# Install flatpak SDKs

```
flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --noninteractive --user flathub \
    org.freedesktop.Platform//23.08 \
    org.freedesktop.Sdk//23.08 \
    org.freedesktop.Sdk.Extension.rust-stable//23.08 \
    org.freedesktop.Sdk.Extension.llvm17//23.08
```

# Build and install app

```
# sudo apt install flatpak-builder git-lfs
# or flatpak install -y flathub org.flatpak.Builder
flatpak uninstall fan-control -y || true

flatpak-builder \
    --force-clean \
    --verbose \
    --ccache \
    --user --install \
    --install-deps-from=flathub \
    --repo=repo \
    flatpak-out \
    com.wiiznokes.fan-control.json
```

# Run app

```
flatpak run com.wiiznokes.fan-control
```

# Udev rules
