#!/bin/bash

# This script is triggered by the json manifest.
# It can also be run manually: flatpak-builder build-dir $id.json

# Exit if any command fails
set -e

# Echo all commands for debug purposes
set -x

# Grab Flatpak build files.
# cp -r org.buhocms.BuhoCMS.desktop .
# cp -r icon.svg .

# Extract portable Flutter build.
mkdir -p BuhoCMS
tar -xf BuhoCMS-Linux-Portable.tar.gz -C BuhoCMS
# rm BuhoCMS/PORTABLE

# Copy the portable app to the Flatpak-based location.
cp -r BuhoCMS /app/
chmod +x /app/BuhoCMS/buhocms
mkdir -p /app/bin
ln -s /app/BuhoCMS/buhocms /app/bin/buhocms

# Install the icon.
iconDir=/app/share/icons/hicolor/scalable/apps
mkdir -p $iconDir
cp -r buhocms.svg $iconDir/org.buhocms.BuhoCMS.svg

# Install the desktop file.
desktopFileDir=/app/share/applications
mkdir -p $desktopFileDir
cp -r org.buhocms.BuhoCMS.desktop $desktopFileDir/

# Install the AppStream metadata file.
metadataDir=/app/share/metainfo
mkdir -p $metadataDir
cp -r org.buhocms.BuhoCMS.metainfo.xml $metadataDir/
