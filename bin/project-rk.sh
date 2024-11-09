#!/bin/env bash

export LAUNCHER_SHA=53c7a4f43ab962cbb7dfc7ab65124b907397adf1f5848b7ac410dc964c211e3c
export C=$WINEPREFIX/drive_c

# Gens wineprefix on first start
(
  if [[ ! -d $WINEPREFIX ]]; then
    umu-run winetricks win7 dxvk allfonts
  fi
) | zenity --progress --title="Project Rubi-Ka" --text="Installing and configuring prefix..." --pulsate --auto-close --percentage=0 --no-cancel --width=360 --height=90

# Gens game launcher if missing/deleted
(
  if [[ ! -d $C/launcher ]]; then
    mkdir -p $C/launcher
    curl -sL https://prk-vault.nyc3.digitaloceanspaces.com/launcher/win.zip -o $C/win.zip
    if [[ $(echo "$LAUNCHER_SHA win.zip" | sha256sum -c) ]]; then
      unzip $C/win.zip -d $C/launcher && rm $C/win.zip
    fi
    chmod +x $C/launcher/*.exe
  fi
) | zenity --progress --title="Project Rubi-Ka" --text="Downloading latest Launcher..." --pulsate --auto-close --percentage=0 --no-cancel --width=360 --height=90

if [[ -f $C/launcher/PRK.Launcher.exe ]]; then
  umu-run $C/launcher/PRK.Launcher.exe
else
  echo 'Error: launcher missing'
  exit 99
fi
