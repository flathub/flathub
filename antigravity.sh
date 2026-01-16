#!/bin/bash

# Wayland flags for better Wayland support
WAYLAND_FLAGS=""
if [[ -n "${WAYLAND_DISPLAY}" ]]; then
    WAYLAND_FLAGS="--enable-features=UseOzonePlatform,WaylandWindowDecorations --ozone-platform=wayland"
fi

# Set up environment for Flatpak
export ELECTRON_TRASH=gio
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

# Create temp directory if it doesn't exist
mkdir -p "${TMPDIR}"

# Run Antigravity with proper flags
exec /app/antigravity/antigravity \
    --no-sandbox \
    --unity-launch \
    ${WAYLAND_FLAGS} \
    "$@"
