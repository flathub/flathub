#!/bin/bash

# Enable Ozone Wayland if the user is running a Wayland session (recommended for Google/Electron apps)
if [ "$XDG_SESSION_TYPE" == "wayland" ]; then
    export NIXOS_OZONE_WL=1
fi

# Launch the app from the standard /app/lib/antigravity/ directory.
# Electron-based apps require --no-sandbox to run properly inside Flatpak's sandbox.
exec /app/lib/antigravity/antigravity --no-sandbox "$@"
