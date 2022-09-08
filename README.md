# qFlipper Flatpak

## Upstream repository

<https://github.com/flipperdevices/qFlipper>

## Udev rules

```
flatpak run --command=cat one.flipperzero.qFlipper /app/lib/udev/rules.d/42-flipperzero.rules | sudo tee /etc/udev/rules.d/42-flipperzero.rules && sudo udevadm control --reload-rules && sudo udevadm trigger
```

## Building the flatpak

### Prerequisites

- flatpak with Flathub repository (setup guide at [Flathub website](https://flatpak.org/setup/))
- flatpak-builder

```bash
# Fedora
sudo dnf install make flatpak-builder

# Ubuntu
sudo apt install make flatpak-builder
```

You will need the following platforms installed:

- `org.kde.Platform` version "5.15-21.08"
- `org.kde.Sdk` version "5.15-21.08"

```bash
flatpak install org.freedesktop.Platform org.freedesktop.Sdk
```