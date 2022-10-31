#!/bin/sh

#
# This wrapper makes a lot of assumptions on how openscad is called
# from within FreeCAD
#

exec flatpak-spawn --directory="$(pwd)" --host flatpak run --filesystem="$XDG_CACHE_HOME/tmp" org.videolan.VLC "$@"
