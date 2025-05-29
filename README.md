# WSelector

A modern GTK4/libadwaita application for browsing and setting wallpapers from Wallhaven.cc.

## Flatpak Installation

### From Flathub (Recommended)
```bash
flatpak install flathub io.github.Cookiiieee.WSelector
```

### Building from Source

#### Prerequisites
- Flatpak
- Flathub repository

```bash
# Add Flathub repository
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Install required SDK
flatpak install flathub org.gnome.Sdk//48 org.gnome.Platform//48

# Build and install
flatpak-builder --user --install --force-clean build-dir io.github.Cookiiieee.WSelector.json

# Run the application
flatpak run io.github.Cookiiieee.WSelector
```

## Features
- Browse wallpapers from Wallhaven.cc
- Search functionality with filters
- Preview wallpapers before setting
- Support for multiple monitor setups
- Dark/Light theme support
- High resolution wallpaper support

## Permissions
This application requires the following permissions:
- Network access (to fetch wallpapers)
- Filesystem access to Pictures directory
- Wayland/X11 display server access

## Troubleshooting
If you encounter any issues, please file a bug at the [GitHub repository](https://github.com/Cookiiieee/WSelector/issues).

## License

[GPL-3.0-or-later](https://www.gnu.org/licenses/gpl-3.0.html)
