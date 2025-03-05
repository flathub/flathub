#!/bin/bash
# Check for FFmpeg in Flatpak extension first
if [ -d "/app/lib/ffmpeg" ]; then
  export PATH="/app/lib/ffmpeg:$PATH"
  export LD_LIBRARY_PATH="/app/lib/ffmpeg:$LD_LIBRARY_PATH"
  echo "Using FFmpeg from Flatpak extension"
fi

# Check for VLC in Flatpak extension
if [ -d "/app/lib/vlc" ]; then
  export VLC_PLUGIN_PATH="/app/lib/vlc/plugins"
  export LD_LIBRARY_PATH="/app/lib/vlc:/app/lib/vlc/plugins:$LD_LIBRARY_PATH"
  echo "Using VLC from Flatpak extension"
else
  echo "Warning: VLC extension not found. Please install the VLC Flatpak extension."
  echo "Run: flatpak install flathub org.videolan.VLC"
  exit 1
fi

# Print FFmpeg version for debugging
if command -v ffmpeg >/dev/null 2>&1; then
  echo "Using FFmpeg: $(which ffmpeg)"
  ffmpeg -version | head -n 1
else
  echo "Warning: FFmpeg not found. Local recording functionality will be unavailable."
fi

# Set XDG_RUNTIME_DIR if not set
if [ -z "$XDG_RUNTIME_DIR" ]; then
  export XDG_RUNTIME_DIR=/tmp/runtime-$USER
  mkdir -p $XDG_RUNTIME_DIR
  chmod 700 $XDG_RUNTIME_DIR
fi

# Make sure Qt can find its plugins
export QT_PLUGIN_PATH=/app/lib/qt5/plugins:/app/lib/plugins:/usr/lib/qt5/plugins

# Try to detect the display environment
if [ -z "$DISPLAY" ] && [ -n "$WAYLAND_DISPLAY" ]; then
  # We're on Wayland
  export QT_QPA_PLATFORM=wayland
  echo "Detected Wayland environment, using wayland platform"
else
  # Default to X11
  export QT_QPA_PLATFORM=xcb
  echo "Using X11 platform"
fi

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