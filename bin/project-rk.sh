#!/bin/env bash

export C=$WINEPREFIX/drive_c

# Gens wineprefix on first start
(
  if [[ ! -d $WINEPREFIX ]]; then
    umu-run winetricks win7 dxvk corefonts
  fi
) | zenity --progress --title="Project Rubi-Ka" --text="Installing and configuring prefix..." --pulsate --auto-close --percentage=0 --no-cancel --width=360 --height=90

# Gens game launcher if missing/deleted
(
  if [[ ! -d $C/launcher ]]; then
    mkdir -p $C/launcher
    curl -sL https://prk-vault.nyc3.digitaloceanspaces.com/launcher/win.zip -o $C/win.zip
    unzip $C/win.zip -d $C/launcher && rm $C/win.zip
    chmod +x $C/launcher/*.exe
  fi
) | zenity --progress --title="Project Rubi-Ka" --text="Downloading latest Launcher..." --pulsate --auto-close --percentage=0 --no-cancel --width=360 --height=90

if [[ -f $C/launcher/PRK.Launcher.exe ]]; then
  umu-run $C/launcher/PRK.Launcher.exe
else
  echo 'Error: launcher missing'
  exit 99
fi
