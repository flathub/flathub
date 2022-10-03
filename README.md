# Putty Flatpak

## Upstream repository

<https://git.tartarus.org/simon/putty.git>

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

- `org.freedesktop.Platform` version "22.08"
- `org.freedesktop.Sdk` version "22.08"

```bash
flatpak install org.freedesktop.Platform org.freedesktop.Sdk
```
