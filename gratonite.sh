#!/bin/bash

# Gratonite Flatpak wrapper script
# Reads user flags, sets up environment, and launches via zypak

FLATPAK_ID=${FLATPAK_ID:-"chat.gratonite.Desktop"}

# Read user-defined Chromium/Electron flags if present
if [ -f "${XDG_CONFIG_HOME}/gratonite-flags.conf" ]; then
    mapfile -t FLAGS <<< "$(grep -Ev '^\s*$|^#' "${XDG_CONFIG_HOME}/gratonite-flags.conf")"
fi

env TMPDIR="${XDG_CACHE_HOME}" zypak-wrapper /app/gratonite/gratonite-desktop --enable-speech-dispatcher "${FLAGS[@]}" "$@"
