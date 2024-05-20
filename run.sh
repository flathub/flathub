#!/bin/sh
declare -a EXTRA_FLAGS=()

# Display Socket
WAYLAND_SOCKET=${WAYLAND_DISPLAY:-"wayland-0"}

if [[ $XDG_SESSION_TYPE == "wayland" && -e "$XDG_RUNTIME_DIR/${WAYLAND_SOCKET}" ]]
then
    
    echo "Wayland socket is available, running natively on Wayland."
    echo "To disable, remove the --socket=wayland permission."
    EXTRA_FLAGS+=(
        "--enable-features=WaylandWindowDecorations"                    
        "--ozone-platform-hint=auto"            
    )        
   
    if [[ -c /dev/nvidia0 ]]
    then
        echo "Using NVIDIA on Wayland, applying workaround"
        EXTRA_FLAGS+=("--disable-gpu-sandbox")
    fi

else
    echo "Wayland socket not available, running through Xwayland."
fi

cd /app/lib/sbom-workbench
export TMPDIR="$XDG_CACHE_HOME"
exec zypak-wrapper ./scanoss-workbench "${EXTRA_FLAGS[@]}" "$@"