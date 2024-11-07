#!/bin/env bash

export C=$WINEPREFIX/drive_c
export PREFIX_DGV=$C/launcher/client/dgVoodooCpl.exe

if [[ -f $PREFIX_DGV ]]; then
  umu-run $PREFIX_DGV # Version in the prefix
else
  install -Dm 755 $(which dgVoodooCpl.exe) $PREFIX_DGV # Install it if it's not there
  umu-run $PREFIX_DGV
fi
