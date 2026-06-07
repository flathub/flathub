#!/bin/sh
# Flatpak entrypoint (installed as /app/bin/Sibir.sh).
set -eu
GAME_DIR="/app/opt/sibir"
cd "${GAME_DIR}"
exec ./Sibir.x86_64 "$@"
