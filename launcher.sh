#!/bin/sh

OLDDIR=`pwd`
LIBDIR=/app/lib/poweriso
export LD_LIBRARY_PATH=$LIBDIR
export QT_QPA_PLATFORM_PLUGIN_PATH=$LIBDIR
# export QT_DEBUG_PLUGINS=1
cd $LIBDIR
exec ./poweriso
