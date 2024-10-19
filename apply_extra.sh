#!/bin/bash

APP_IMAGE="/app/extra/Trezor-Suite.AppImage"

# Allow image to execute

chmod +x $APP_IMAGE

# Extract image

unappimage $APP_IMAGE

# Install data
DEST="/app/extra/bin/"
mkdir $DEST
cp -r squashfs-root/* $DEST



# Clean up
rm -rf squashfs-root/
rm $APP_IMAGE