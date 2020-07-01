#!/bin/sh

# Helper script to copy all the Chrome icons from a host installation

for icon in /usr/share/icons/hicolor/*/apps/google-chrome.png; do
  size=$(echo $icon | grep -Po '\d+x\K\d+')
  cp $icon com.google.Chrome-$size.png
done
