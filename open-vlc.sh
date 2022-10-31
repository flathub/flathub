#!/bin/sh
exec flatpak-spawn --directory="$(pwd)" --host flatpak run --filesystem="$XDG_CACHE_HOME/tmp" org.videolan.VLC "$@"
