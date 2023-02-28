#!/bin/bash


# Convert the archive of the Flutter app to a Flatpak.


# Exit if any command fails
set -e

# Echo all commands for debug purposes
set -x


# No spaces in project name.
projectName=qr
projectId=dev.lasheen.qr
executableName=qr


# ------------------------------- Build Flatpak ----------------------------- #

# Extract portable Flutter build.
mkdir -p $projectName
tar -xf $projectName-Linux-Portable.tar.gz -C $projectName

# Copy the portable app to the Flatpak-based location.
cp -r $projectName /app/
chmod +x /app/$projectName/$executableName
mkdir -p /app/bin
ln -s /app/$projectName/$executableName /app/bin/$executableName

# Install the icon.
iconDir=/app/share/icons/hicolor/scalable/apps
mkdir -p $iconDir
cp -r assets/icons/$projectId.svg $iconDir/

# Install the desktop file.
desktopFileDir=/app/share/applications
mkdir -p $desktopFileDir
cp -r packaging/$projectId.desktop $desktopFileDir/

# Install the AppStream metadata file.
metadataDir=/app/share/metainfo
mkdir -p $metadataDir
cp -r packaging/$projectId.metainfo.xml $metadataDir/