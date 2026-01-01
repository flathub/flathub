#!/bin/bash
# AnalogStudio launcher script for Flathub

set -e

# Export necessary environment variables for Electron
export ELECTRON_OZONE_PLATFORM_HINT=auto
export PATH=/app/bin:$PATH
export LD_LIBRARY_PATH=/app/lib:$LD_LIBRARY_PATH

# Launch AnalogStudio
exec /app/bin/electron /app/share/analogstudio "$@"
