#!/usr/bin/bash
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
#exec zypak-wrapper /app/extra/brave "$@"
exec /app/extra/brave --no-sandbox "$@"
