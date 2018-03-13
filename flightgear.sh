#!/bin/sh
FG_ROOT=/app/share/flightgear LD_LIBRARY_PATH=/app/lib64 FG_AIRCRAFT=~/.fgfs/Aircraft/ FG_SCENERY=~/.fgfs/Scenery fgfs "$@"
