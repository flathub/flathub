#!/bin/bash
# BrowserOS Flatpak Wrapper Script
# Based on AppRun from BrowserOS AppImage

set -e

# Set up environment variables from original AppRun
export LD_LIBRARY_PATH="/app/opt/browseros:$LD_LIBRARY_PATH"
export CHROME_WRAPPER="/app/bin/browseros-wrapper.sh"

# Handle flatpak-specific paths and environment
export XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-$HOME/.cache}"
export XDG_DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"

exec /app/opt/browseros/browseros \
  --no-sandbox \
  --test-type \
  --disable-dev-shm-usage \
  "$@"

