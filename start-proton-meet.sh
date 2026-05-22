#!/bin/sh

# Set separate sandbox temp directory
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"

# Check if session is Wayland
if [ "${XDG_SESSION_TYPE}" = "wayland" ]; then
    # Start with Wayland support enabled for Electron/Ozone
    exec zypak-wrapper "/app/proton-meet/Proton Meet Beta" \
        --ozone-platform-hint=auto \
        --enable-features=WaylandWindowDecorations \
        "$@"
else
    # Fallback to X11/Xwayland
    exec zypak-wrapper "/app/proton-meet/Proton Meet Beta" "$@"
fi
