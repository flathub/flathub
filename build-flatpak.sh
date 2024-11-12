#!/bin/bash

# Exit if any command fails
set -e

# Echo all commands for debug purposes
set -x

projectBundleDir=tubesync
projectId=io.github.khaled_0.TubeSync
executableName=tubesync

# Unzip Binary
mkdir $projectBundleDir
unzip $projectBundleDir.zip -d $projectBundleDir

# Copy the portable app to the Flatpak-based location.
cp -r $projectBundleDir /app/
chmod +x /app/$projectBundleDir/$executableName
mkdir -p /app/bin
ln -s /app/$projectBundleDir/$executableName /app/bin/$executableName

# Install the icon.
iconDir=/app/share/icons/hicolor/scalable/apps
mkdir -p $iconDir
cp -r $projectBundleDir/data/flutter_assets/assets/tubesync.png $iconDir/$projectId.png

# Install the desktop file.
desktopFileDir=/app/share/applications
mkdir -p $desktopFileDir
cp -r ./$projectId.desktop $desktopFileDir/

# Install the AppStream metadata file.
metadataDir=/app/share/metainfo
mkdir -p $metadataDir
cp -r ./$projectId.metainfo.xml $metadataDir/
