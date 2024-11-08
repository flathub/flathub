#!/bin/env bash

export C=$WINEPREFIX/drive_c
export PREFIX_DGV=$C/launcher/client/dgVoodooCpl.exe

if [[ -f $PREFIX_DGV ]]; then
  umu-run $PREFIX_DGV
else
  echo "dgVoodooCpl.exe not found"
fi
