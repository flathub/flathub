#!/bin/bash
# Check for FFmpeg in Flatpak extension first
if [ -d "/app/lib/ffmpeg" ]; then
  export PATH="/app/lib/ffmpeg:$PATH"
  export LD_LIBRARY_PATH="/app/lib/ffmpeg:$LD_LIBRARY_PATH"
  echo "Using FFmpeg from Flatpak extension"
fi




# Make sure Qt can find its plugins
export QT_PLUGIN_PATH=/app/lib/qt5/plugins:/app/lib/plugins:/usr/lib/qt5/plugins


# Debug info
echo "Display: $DISPLAY"
echo "Wayland Display: $WAYLAND_DISPLAY"
echo "XDG_RUNTIME_DIR: $XDG_RUNTIME_DIR"
echo "QT_PLUGIN_PATH: $QT_PLUGIN_PATH"
echo "QT_QPA_PLATFORM: $QT_QPA_PLATFORM"
echo "VLC_PLUGIN_PATH: $VLC_PLUGIN_PATH"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

# Run the application
cd /app/share/tvhplayer
python3 tvhplayer.py "$@" 