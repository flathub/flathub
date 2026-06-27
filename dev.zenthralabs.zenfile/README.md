# ZenFile Flatpak Packaging Guide

This directory contains the files needed to package ZenFile as a Flatpak and publish it on Flathub (making it discoverable in the COSMIC Store, GNOME Software, Discover, etc.).

## Prerequisites

Make sure you have `flatpak` and `flatpak-builder` installed:
```bash
sudo apt install flatpak flatpak-builder
```

## Step-by-Step Build & Test Guide

### 1. Place your App Icon
Put your app icon (512x512 resolution, PNG format) in this directory and name it:
`dev.zenthralabs.zenfile.png`

### 2. Generate Cargo Offline Sources
Flatpak builds in a sandbox without internet access. You must generate a list of all Rust crate sources from your `Cargo.lock`.

Run the [flatpak-cargo-generator.py](https://github.com/flathub/flatpak-builder-tools/blob/master/cargo/flatpak-cargo-generator.py) script:
```bash
# Download the helper generator
curl -o flatpak-cargo-generator.py https://raw.githubusercontent.com/flathub/flatpak-builder-tools/master/cargo/flatpak-cargo-generator.py

# Run it on your Cargo.lock
python3 flatpak-cargo-generator.py ../Cargo.lock -o generated-sources.json
```

### 3. Build the Flatpak Locally
Run `flatpak-builder` to compile ZenFile inside the sandbox:
```bash
flatpak-builder --force-clean --user --install build-dir dev.zenthralabs.zenfile.yml
```

### 4. Run the Sandbox App
Test the newly built sandboxed app to verify everything behaves correctly:
```bash
flatpak run dev.zenthralabs.zenfile
```
