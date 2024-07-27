#!/bin/bash
export TMPDIR=$XDG_CACHE_HOME/tmp

MOZ_LAUNCHER="$MOZ_DIST_BIN/launch-script.sh"
##
## Enable Wayland backend?
##
if ! [ $MOZ_DISABLE_WAYLAND ] && [ "$WAYLAND_DISPLAY" ]; then
  if [ "$XDG_CURRENT_DESKTOP" == "GNOME" ]; then
    export MOZ_ENABLE_WAYLAND=1
  fi
##  Enable Wayland on KDE/Sway
##
  if [ "$XDG_SESSION_TYPE" == "wayland" ]; then
    export MOZ_ENABLE_WAYLAND=1
  fi
fi

exec /app/bin/zen "$@"