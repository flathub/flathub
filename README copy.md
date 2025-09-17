# Glide App

A Qt-based file transfer application with GUI and command-line tools.

## Features
- GUI interface for easy file transfers
- Command-line client and server tools  
- Multi-language support (Arabic, English, Russian)
- Cross-platform compatibility via Flatpak

## Installation

### From GitHub Releases (Recommended)
1. Go to [Releases](https://github.com/kareem2099/Glide/releases)
2. Download the latest `glide.flatpak` file
3. Install: `flatpak install glide.flatpak`
4. Run: `flatpak run org.glideapp.GlideApp`

### Prerequisites
- Flatpak installed on your system
- KDE runtime: `flatpak install flathub org.kde.Platform//6.9`

## Building from Source
```bash
# Clone the repository
git clone https://github.com/kareem2099/Glide.git
cd Glide

# Build Flatpak
flatpak-builder --force-clean --repo=repo flatpak_build org.glideapp.GlideApp.yaml

# Install locally
flatpak --user remote-add --no-gpg-verify glide-repo repo
flatpak --user install glide-repo org.glideapp.GlideApp
