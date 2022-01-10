#!/bin/bash -e

SRC="/app/extra/wt.tar.gz"

# Unload launcher into data directory
tar -xv --gzip -f $SRC -C $XDG_DATA_HOME

# Execute launcher
exec "${XDG_DATA_HOME}/WarThunder/launcher"
