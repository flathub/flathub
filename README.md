# WSelector

A modern GTK4/libadwaita application for browsing and setting wallpapers from Wallhaven.cc.

## Features
- Browse wallpapers from Wallhaven.cc
- Search functionality with filters
- Preview wallpapers before setting
- Support for multiple monitor setups
- Dark/Light theme support

## Building Locally

To build and test this Flatpak locally:

```bash
# Install the required SDK
flatpak install flathub org.gnome.Sdk//48 org.gnome.Platform//48

# Build and install
flatpak-builder --user --install --force-clean build-dir io.github.Cookiiieee.WSelector.json

# Run the application
flatpak run io.github.Cookiiieee.WSelector
```

## License

GPL-3.0-or-later
