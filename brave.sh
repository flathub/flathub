#!/usr/bin/sh
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec /app/extra/brave-browser --test-type --no-sandbox "$@"
