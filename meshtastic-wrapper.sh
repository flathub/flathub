#!/usr/bin/env bash
export JAVA_HOME=/app/jre

# Create config directory if it doesn't exist
mkdir -p "${XDG_CONFIG_HOME:-$HOME/.config}/meshtastic-desktop"

# Use a writable tmpdir within the Flatpak sandbox
export TMPDIR="${XDG_CACHE_HOME:-$HOME/.cache}/tmp"
mkdir -p "$TMPDIR"

# Set Java preferences location to use Flatpak's config directory
exec /app/jre/bin/java \
    -Djava.io.tmpdir="$TMPDIR" \
    -Djava.awt.headless=false \
    -Dawt.useSystemAAFontSettings=on \
    -Dswing.aatext=true \
    -Djava.util.prefs.userRoot="${XDG_CONFIG_HOME:-$HOME/.config}/meshtastic-desktop" \
    -jar /app/lib/meshtastic-desktop.jar "$@"
