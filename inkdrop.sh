#!/bin/sh
set -eu

WL_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
if [ -e "${XDG_RUNTIME_DIR}/${WL_DISPLAY}" ] || [ -e "/${WL_DISPLAY}" ]; then
  set -- --ozone-platform-hint=auto \
    --enable-features=WaylandWindowDecorations \
    --enable-wayland-ime \
    --wayland-text-input-version=3 \
    "$@"
else
  set -- --ozone-platform=x11 "$@"
fi

exec zypak-wrapper /app/extra/inkdrop "$@"
