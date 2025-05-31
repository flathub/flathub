# WSelector

WSelector, a modern GTK4/libadwaita application for browsing and setting wallpapers from Wallhaven.cc.

## Installation

### From Flathub (Recommended)

1. Add the Flathub repository if you haven't already:
   ```bash
   flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
   ```

2. Install WSelector:
   ```bash
   flatpak install flathub io.github.Cookiiieee.WSelector
   ```

3. Run the application:
   ```bash
   flatpak run io.github.Cookiiieee.WSelector
   ```

### Building from source

1. Install Flatpak and Flatpak Builder:
   ```bash
   sudo apt install flatpak flatpak-builder
   flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
   ```

2. Clone this repository:
   ```bash
   git clone https://github.com/flathub/io.github.Cookiiieee.WSelector.git
   cd io.github.Cookiiieee.WSelector
   ```

3. Build and install:
   ```bash
   flatpak-builder --user --install --force-clean build-dir io.github.Cookiiieee.WSelector.json
   ```

4. Run the application:
   ```bash
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
