#!/bin/bash
# TODO: Figure out why Wayland doesn't work properly
# WAYLAND_SOCKET=${WAYLAND_DISPLAY:-"wayland-0"}

# if [[ -e "$XDG_RUNTIME_DIR/${WAYLAND_SOCKET}" || -e "${WAYLAND_DISPLAY}" ]]
# then
#     FLAGS="--ozone-platform-hint=auto --enable-features=WaylandWindowDecorations"
# fi

env TMPDIR=$XDG_CACHE_HOME zypak-wrapper /app/exponential/electron $FLAGS "$@"
