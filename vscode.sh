#!/usr/bin/bash

flatpak='flatpak-spawn --host flatpak'

for ref in com.visualstudio.code.oss com.visualstudio.code; do
  if $flatpak info -m $ref >/dev/null 2>&1; then
    exec $flatpak run $ref "$@"
  fi
done

zenity --error --no-wrap --title="Visual Studio Code is not installed" \
  --text="Visual Studio Code is required to edit scripts with the Unity Hub Flatpak, please \
install it from Flathub."
