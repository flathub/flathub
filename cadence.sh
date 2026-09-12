#!/usr/bin/env bash
set -e

export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID:-io.github.DRNZY.Cadence}"
mkdir -p "$TMPDIR"

FLAGS=(
  "--enable-gpu-rasterization"
  "--enable-zero-copy"
  "--enable-gpu-compositing"
  "--enable-native-gpu-memory-buffers"
  "--enable-features=UseSkiaRenderer"
)

# Wayland display socket auto-detection
if [ -n "$WAYLAND_DISPLAY" ]; then
  FLAGS+=("--ozone-platform-hint=auto")
fi

exec zypak-wrapper /app/cadence/cadence "${FLAGS[@]}" "$@"
