#!/bin/sh
set -e

# In Flatpak, default XDG dirs point to ~/.var/app/.../ (paths containing leading dot '.var').
# Tauri's asset protocol security scope excludes hidden directories by default ("**/**").
# Using /var/ (the system sandbox mount without leading dots) allows Tauri's asset protocol
# to safely serve application data like map thumbnails and offline fonts.
export XDG_DATA_HOME="/var/data"
export XDG_CONFIG_HOME="/var/config"
export XDG_CACHE_HOME="/var/cache"

mkdir -p "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME"

exec /app/bin/MindElixir.bin "$@"
