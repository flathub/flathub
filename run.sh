#!/bin/sh

declare -a EXTRA_FLAGS=()

# Display Socket
if [[ $XDG_SESSION_TYPE == "wayland" ]]
then
    WAYLAND_SOCKET=${WAYLAND_DISPLAY:-"wayland-0"}

    if [[ -e "$XDG_RUNTIME_DIR/${WAYLAND_SOCKET}" ]]
    then
        echo "Wayland socket found, running via Wayland."
        echo "To run via XWayland, remove the --socket=wayland permission."
        EXTRA_FLAGS+=(
            "--enable-features=WaylandWindowDecorations"
            "--ozone-platform=wayland"
        )
    else
        echo "No Wayland permission, running via XWayland."
    fi
    if [[ -c /dev/nvidia0 ]]
    then
        echo "Using NVIDIA on Wayland, applying workaround"
        EXTRA_FLAGS+=("--disable-gpu-sandbox")
    fi

fi

cd /app/lib/ThinkerCAD-linux-x64
export TMPDIR="$XDG_CACHE_HOME"
exec zypak-wrapper ./ThinkerCAD "${EXTRA_FLAGS[@]}" "$@"
