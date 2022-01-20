#!/bin/bash -e

APP_IMAGE="/app/extra/lunar-client-2.9.3.appimage"

# Allow image to execute
chmod +x $APP_IMAGE

# Move into tmp dir
RUN_DIR=/app/extra/tmp/
mkdir $RUN_DIR
cd $RUN_DIR

# Extract image
exec "$APP_IMAGE" --appimage-extract

# Install data
mkdir "${FLATPAK_DEST}/lunarclient"
cp -r squashfs-root/* "${FLATPAK_DEST}/lunarclient/"

mkdir -p "${FLATPAK_DEST}/share/icons/hicolor/"
cp -r squashfs-root/usr/share/icons/hicolor/* "${FLATPAK_DEST}/extra/export/share/icons/hicolor/"

# Clean up

rm -rf $RUN_DIR
