#!/bin/sh

#
# This wrapper makes a lot of assumptions on how openscad is called
# from within FreeCAD
#

BASEDIR="$XDG_CACHE_HOME/tmp"

if test "$1" != "-o"; then
    exec flatpak-spawn --host flatpak run --filesystem="$BASEDIR" org.openscad.OpenSCAD $*
else
    REMOTE_OUT="$BASEDIR/`basename "$2"`"
    exec flatpak-spawn --directory=`pwd` --host flatpak run --filesystem="$BASEDIR" org.openscad.OpenSCAD -o "$REMOTE_OUT" "$3"
fi
