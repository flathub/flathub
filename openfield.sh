#!/bin/sh
# Openfield ships closed-source — Flathub can't compile it in-sandbox, so
# the AppImage is fetched as a flatpak `extra-data` source (same pattern
# Spotify/Slack/Discord use for their proprietary Flatpaks) and lands
# read-only at /app/extra/openfield.AppImage. It needs its own +x bit to
# run, which a read-only /app can't hold, so we copy it once into the
# app's writable cache dir, chmod there, then run it with
# --appimage-extract-and-run (works without FUSE, which the sandbox lacks).
set -e
CACHE_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/io.github.NatanBack77.Openfield-Releases"
SRC="/app/extra/openfield.AppImage"
DST="$CACHE_DIR/openfield.AppImage"

mkdir -p "$CACHE_DIR"
SRC_SIZE=$(stat -c%s "$SRC" 2>/dev/null || echo 0)
DST_SIZE=$(stat -c%s "$DST" 2>/dev/null || echo -1)
if [ "$SRC_SIZE" != "$DST_SIZE" ]; then
  cp "$SRC" "$DST"
  chmod +x "$DST"
fi

exec "$DST" --appimage-extract-and-run "$@"
