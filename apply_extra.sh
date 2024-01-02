#!/bin/bash

# Extract the appimage
chmod +x bruno.appimage
unappimage bruno.appimage >/dev/null

# Move all the data to /app/extra/bruno
mv squashfs-root bruno

# Clean up
rm bruno.appimage