#!/bin/sh
# Lanceur VoxCut dans le sandbox Flatpak.
# Le binaire PyInstaller (onedir) est décompressé dans /app/extra/VoxCut par apply_extra.
exec /app/extra/VoxCut/VoxCut "$@"
