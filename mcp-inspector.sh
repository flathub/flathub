#!/bin/sh

export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"

if [ "${XDG_SESSION_TYPE}" = "wayland" ]; then
    zypak-wrapper /app/lib/mcp-inspector/@rolaca11mcp-inspector-electron --enable-features=UseOzonePlatform --ozone-platform=wayland "$@"
else
    zypak-wrapper /app/lib/mcp-inspector/@rolaca11mcp-inspector-electron "$@"
fi
