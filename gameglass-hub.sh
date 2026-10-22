#!/bin/sh
set -e

# Keep Electron's scratch files inside the sandbox's runtime dir.
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
mkdir -p "$TMPDIR"

# Electron only picks Wayland when told to; without the hint it falls back to
# XWayland and renders blurry on HiDPI.
exec zypak-wrapper /app/gameglass-hub/gameglass-hub \
  --ozone-platform-hint=auto \
  --enable-features=WaylandWindowDecorations \
  "$@"
