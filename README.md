# Enigma

## Upstream repository

<https://github.com/Enigma-Game/Enigma>

## Building the flatpak

### Prerequisites

- flatpak with Flathub repository (setup guide at [Flathub website](https://flatpak.org/setup/))
- flatpak-builder
- make (optional)

```bash
# Fedora
sudo dnf install make flatpak-builder

# Ubuntu
sudo apt install make flatpak-builder
```

You will need the following platforms installed:

- org.freedesktop.Platform
- org.freedesktop.Sdk

```bash
flatpak install org.freedesktop.Platform org.freedesktop.Sdk
```

### Compile

```bash
make
```

or if you don't have make installed

```bash
flatpak-builder --ccache --force-clean --state-dir=build/flatpak-builder --repo=build/flatpak-repo build/flatpak-target org.nongnu.enigma
```

### Installing

```bash
make install
```

or if you don't have make installed

```bash
flatpak install --reinstall --or-update -y --user ./build/flatpak-repo org.nongnu.enigma
```

### Export bundle

```bash
make dist
```

or if you don't have make installed

```bash
flatpak build-bundle build/flatpak-repo org.nongnu.enigma.flatpak org.nongnu.enigma
```