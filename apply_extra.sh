#!/bin/bash -e

SRC="/app/extra/wt.tar.gz"
XDG_DATA_HOME="/app/extra/tmp/"

# Setup
mkdir -p $XDG_DATA_HOME

# Extract
tar -xv --gzip -f $SRC -C $XDG_DATA_HOME

# Export icon
mkdir -p "/app/extra/export/share/icons/hicolor/128x128/apps/"
mv -v "$XDG_DATA_HOME/WarThunder/launcher.ico" "/app/extra/export/share/icons/hicolor/128x128/apps/net.gaijin.WarThunder.png"

# Clean up
rm -rf $XDG_DATA_HOME
