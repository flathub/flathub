#!/usr/bin/bash
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec /app/extra/brave --test-type --no-sandbox "$@"
