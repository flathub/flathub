#!/bin/bash
set -eu

ICONS_DIR=${FLATPAK_DEST}/share/icons/hicolor
SVG=$ICONS_DIR/scalable/apps/speedcrunch.svg
SIZES="16 22 24 32 48 64 128 256"

for s in $SIZES; do
    d=$ICONS_DIR/${s}x${s}/apps
    mkdir --mode=0755 -p $d
    rsvg-convert -w $s -h $s $SVG -o $d/speedcrunch.png
    chmod 0644 $d/speedcrunch.png
done
