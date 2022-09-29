# qFlipper Flatpak

## Upstream repository

<https://github.com/flipperdevices/qFlipper>

## Fix permissions error on serial port

Check out the [wiki page on how to troubleshoot the connection to Flipper](https://github.com/flathub/one.flipperzero.qFlipper/wiki/)

## Building the flatpak

### Prerequisites

- flatpak with Flathub repository (setup guide at [Flathub website](https://flatpak.org/setup/))
- flatpak-builder

```bash
# Fedora
sudo dnf install flatpak-builder

# Ubuntu
sudo apt install flatpak-builder
```

You will need the following platforms installed:

- `org.kde.Platform` version "5.15-21.08"
- `org.kde.Sdk` version "5.15-21.08"

```bash
flatpak install org.freedesktop.Platform org.freedesktop.Sdk
```
