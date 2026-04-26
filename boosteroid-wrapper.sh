#!/bin/sh
set -eu

BASE="/app/extra/opt/BoosteroidGamesS.R.L./bin"
BIN="$BASE/Boosteroid"

if [ ! -x "$BIN" ]; then
  echo "Boosteroid payload missing. Reinstall Flatpak to re-run apply_extra." >&2
  exit 1
fi

# Ensure desktop integration assets from extracted package can be resolved.
export XDG_DATA_DIRS="/app/extra/usr/share:${XDG_DATA_DIRS:-/app/share:/usr/share}"
export LD_LIBRARY_PATH="/app/extra/usr/local/lib:${LD_LIBRARY_PATH:-}"

if [ -n "${WAYLAND_DISPLAY:-}" ] && [ -n "${DISPLAY:-}" ]; then
  export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland;xcb}"
  # Boosteroid's Qt build often fails on wayland-egl in Flatpak; shm keeps Wayland stable.
  export QT_WAYLAND_CLIENT_BUFFER_INTEGRATION="${QT_WAYLAND_CLIENT_BUFFER_INTEGRATION:-shm}"
elif [ -n "${WAYLAND_DISPLAY:-}" ]; then
  export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland}"
  export QT_WAYLAND_CLIENT_BUFFER_INTEGRATION="${QT_WAYLAND_CLIENT_BUFFER_INTEGRATION:-shm}"
elif [ -n "${DISPLAY:-}" ]; then
  export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
else
  echo "No graphical display available (WAYLAND_DISPLAY/DISPLAY not set)." >&2
  exit 1
fi

exec "$BIN" "$@"
