# Tibia Flatpak

Tibia MMORPG client.

Verdana fonts are included in the package. 

This community-provided package is not verified by, affiliated with, or supported by CipSoft GmbH.

## Building Locally

### Prerequisites
- `flatpak` installed
- `flatpak-builder` installed

### Build

```bash

# Build the package
flatpak-builder --user --install --force-clean .build com.tibia.client.yml

# Run
flatpak run com.tibia.client

## Updating package

# Update runtime
flatpak install flathub org.freedesktop.Platform//<sdk_version>
flatpak install flathub org.freedesktop.Sdk//<sdk_version>
flatpak install flathub org.freedesktop.Sdk.Extension.llvm<llvm_version>//<sdk_version>

# Update com.tibia.client.yml
- sdk version 
- llvm version
- tibia download URL sha256

# Update com.tibia.client.metainfo.xml
- release version
- release date
