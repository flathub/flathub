#!/bin/sh
# Shim script for xdg-mime.

if [[ "$1" == "query" && "$2" == "default" && "$3" == "x-scheme-handler/fcade" ]]; then
  echo "com.fightcade.Fightcade.fcade-quark.desktop"
  exit 0
fi
