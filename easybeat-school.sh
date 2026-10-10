#!/bin/sh
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec zypak-wrapper /app/easybeat/easybeat-school --ozone-platform=x11 "$@"
