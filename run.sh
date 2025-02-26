#!/bin/bash

# Set scaling factor if not already set
if [ -z "$GDK_SCALE" ]; then
  export GDK_SCALE=1
fi

WAYLAND_SOCKET=${WAYLAND_DISPLAY:-"wayland-0"}

if [[ -e "$XDG_RUNTIME_DIR/${WAYLAND_SOCKET}" || -e "${WAYLAND_DISPLAY}" ]]
then
    FLAGS+=('--enable-features=WaylandWindowDecorations' '--ozone-platform-hint=auto')
fi

# Add default electron flags
FLAGS+=(
    '--ozone-platform-hint=auto'
    '--enable-features=WaylandWindowDecorations'
    '--enable-wayland-ime'
)

env TMPDIR="${XDG_CACHE_HOME}" zypak-wrapper /app/cherrystudio/cherrystudio "${FLAGS[@]}" "$@"