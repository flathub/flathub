#!/bin/sh
set -e
export TMPDIR="$XDG_RUNTIME_DIR/app/${FLATPAK_ID}"

exec zypak-wrapper /app/lib/online-movie-catalog/online-movie-catalog "$@"
