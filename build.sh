#!/bin/bash

# Make errors fatal, print commands
set -ex

# Move to the repository root
cd "$(dirname "$0")"

# Install the required Flatpak runtime and SDK
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub --user org.kde.Sdk//6.10 -y
flatpak install flathub --user org.kde.Platform//6.10 -y
flatpak install flathub --user io.qt.PySide.BaseApp//6.10 -y
flatpak install flathub --user org.freedesktop.Sdk.Extension.openjdk8//25.08 -y

# Build the Flathub package
rm -rf target/ # Don't copy all the planet into the Flatpak build dir
rm -rf repo/
flatpak-builder --install repo re.fossplant.pbtk.json --user -y
