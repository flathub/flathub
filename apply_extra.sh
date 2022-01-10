#!/bin/bash -e

SRC="/app/extra/wt.tar.gz"

# Unload launcher into data directory
echo $SRC
echo $XDG_DATA_HOME

tar -xv --gzip -f $SRC -C $XDG_DATA_HOME

# Export icon
mkdir "/app/extra/export/share/icons/128x128/apps/"
mv -v "$XDG_DATA_HOME/WarThunder/launcher.ico" "/app/extra/export/share/icons/128x128/apps/net.gaijin.WarThunder.png"
