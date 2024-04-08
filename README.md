


# Generate rust sources

```
git clone https://github.com/wiiznokes/fan-control.git --branch flatpak
git clone https://github.com/zecakeh/flatpak-builder-tools.git --branch cargo-submodules
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

# Build app

```
# sudo apt install flatpak-builder git-lfs
flatpak-builder \
    --force-clean \
    packages \
    com.wiiznokes.fan-control.json
```

# Run app

```

```

# Udev rules

