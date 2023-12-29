#!/bin/bash

# Extract the appimage
chmod +x httpie.appimage
unappimage httpie.appimage

# Move all the data to /app/extra/httpie
mv squashfs-root httpie

# Install icons
for size in 16x16 32x32 64x64 128x128 256x256 512x512 1024x1024; do
    install -Dm644 "httpie/usr/share/icons/hicolor/${size}/apps/httpie.png" "/app/extra/export/share/icons/hicolor/${size}/apps/io.httpie.Httpie.png"
done

# Clean up
rm httpie.appimage
