#!/bin/sh
export OLD_DISPLAY="$DISPLAY"
export OLD_WAYLAND_DISPLAY="$WAYLAND_DISPLAY"

export DISPLAY=""
export WAYLAND_DISPLAY=""

/app/bin/wine/bin/wineboot -i
/app/bin/wine/bin/wineserver -k

export DISPLAY="$OLD_DISPLAY"
export WAYLAND_DISPLAY="$OLD_WAYLAND_DISPLAY"

/app/bin/wine/bin/wine '/app/bin/game/Mine Blocks.exe'
