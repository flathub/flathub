#!/bin/sh

EXTRA_FLAGS=()

# Display Socket
if [ "${XDG_SESSION_TYPE}" = "wayland" ] && [ -e "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY:-wayland-0}" ]; then
    EXTRA_FLAGS+=(
        "--enable-features=WaylandWindowDecorations"
        "--ozone-platform-hint=auto"
    )
fi

export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}" 
exec zypak-wrapper /app/materialious/materialious "${EXTRA_FLAGS[@]}" "$@"