#!/bin/sh

# Helper script to copy all the Chrome icons from a host installation

for icon in /usr/share/icons/hicolor/*/apps/com.microsoft.Edge; do
  size=$(echo $icon | grep -Po '\d+x\K\d+')
  cp $icon com.microsoft.Edge-$size.png
done
