#!/bin/sh
# Runs the app unpacked by apply_extra under zypak so Chromium's own sandbox
# keeps working inside the Flatpak sandbox (no --no-sandbox).
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
export GTK_USE_PORTAL=1
mkdir -p "$TMPDIR"
exec zypak-wrapper /app/extra/sai-statistics/sai-statistics "$@"
