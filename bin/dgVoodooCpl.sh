#!/bin/env bash

export C=$WINEPREFIX/drive_c
export CLIENT_PATH=$C/launcher/client

if [[ ! -f $C/launcher/client/dgVoodooCpl.exe ]]; then
  echo 'dgVoodooCpl.exe not installed in the prefix yet!'
else
  umu-run $CLIENT_PATH/dgVoodooCpl.exe
fi
