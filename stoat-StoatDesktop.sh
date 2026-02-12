#!/bin/sh
WAYLAND_FLAGS=""
if [ -n "$WAYLAND_DISPLAY" ]; then
  WAYLAND_FLAGS="--ozone-platform=wayland --enable-features=UseOzonePlatform"
fi

exec zypak-wrapper /app/stoat-desktop/stoat-desktop $WAYLAND_FLAGS "$@"
