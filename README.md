# com.insynchq.Insync Flatpak

Flatpak repackaging for [Insync](https://www.insynchq.com/) (proprietary).

> **Note:** This is a community-maintained package. It is not officially supported by the upstream developers.

## Requirements

- [flatpak](https://flatpak.org/setup/)
- [org.flatpak.Builder](https://flathub.org/apps/org.flatpak.Builder)

## Build and install locally

```bash
# Add the Flathub remote (needed for runtime dependencies)
flatpak remote-add --if-not-exists --user flathub \
  https://dl.flathub.org/repo/flathub.flatpakrepo

# Install the sandboxed builder, runtime, and SDK
flatpak install --user flathub org.flatpak.Builder
flatpak install --user flathub org.freedesktop.Platform/x86_64/25.08
flatpak install --user flathub org.freedesktop.Sdk/x86_64/25.08

# Build to a local repo (do NOT use --install here; apply_extra must run
# outside the Builder sandbox to create the required user namespaces)
flatpak run org.flatpak.Builder --user --repo=repo --force-clean builddir com.insynchq.Insync.yml

# Add the local repo and install from it (runs apply_extra correctly)
flatpak --user remote-add --no-gpg-verify --if-not-exists insync-local repo
flatpak --user install --reinstall insync-local com.insynchq.Insync

# Run
flatpak run com.insynchq.Insync
```

## Lint

```bash
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.insynchq.Insync.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

## Known problems

- Autostart set from within Insync does not work. Use the following config in
  `~/.config/autostart/insync.desktop` instead:

```ini
[Desktop Entry]
Type=Application
Name=com.insynchq.Insync
X-XDP-Autostart=com.insynchq.Insync
Exec=flatpak run com.insynchq.Insync
X-Flatpak=com.insynchq.Insync
X-GNOME-Autostart-Delay=3
```
