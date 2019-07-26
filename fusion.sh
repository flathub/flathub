#!/bin/sh

# *FIXME* cry
HOME="$XDG_CONFIG_HOME"

kega_libdir="/app/lib/KegaFusion"
kega_localdir="$HOME/.Kega Fusion"

# create local plugins directory if not present
mkdir -p "$kega_localdir/Plugins"

# create links for every included plugin
for i in $kega_libdir/plugins/*; do
  if [ ! -e "$kega_localdir/Plugins/$(basename "$i")" ]; then
    ln -sf "$i" "$kega_localdir/Plugins/"
  fi
done

# copy configuration file if not present
if ! [ -f "$kega_localdir/Fusion.ini" ]; then
  cp $kega_libdir/Fusion.ini "$kega_localdir"
fi

# here we go!
$kega_libdir/Fusion "$@"
