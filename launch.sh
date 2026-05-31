#!/bin/sh
# Flatpak entrypoint (installed as /app/bin/Sibir.sh).
set -eu
GAME_DIR="/app/opt/sibir"
if [ ! -x "${GAME_DIR}/Sibir.x86_64" ]; then
	echo "Sibir: game files missing. If this is an extra-data build, finish install from Flathub." >&2
	exit 1
fi
cd "${GAME_DIR}"
exec ./Sibir.x86_64 "$@"
