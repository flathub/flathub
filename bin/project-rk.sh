#!/bin/env bash

export C=$WINEPREFIX/drive_c

if [[ ! -d $WINEPREFIX ]]; then
  umu-run winetricks dxvk win7 corefonts
  (/app/vkd3d-proton/setup_vkd3d_proton.sh install)
fi

if [[ ! -d $C/launcher ]]; then
  mkdir -p $C/launcher
  curl -sL https://prk-vault.nyc3.digitaloceanspaces.com/launcher/win.zip -o $C/win.zip
  unzip $C/win.zip -d $C/launcher
  rm $C/win.zip
  mkdir -p $C/launcher/client
  cp -r /app/dgvoodoo2/* $C/launcher/client/
  chmod +x $C/launcher/*.exe
fi

umu-run $C/launcher/PRK.Launcher.exe
