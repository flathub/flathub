#!/bin/sh
# Flatpak entry point: /app/bin/visioncli, and /app/bin/visioncli-mcp (a link to this) for the MCP server.
# The host's own config, data and state folders (visible through --filesystem=home), not ~/.var/app: the CLIs and
# the MCP server they start run on the host and read ~/.config/visioncli, and a .deb/AppImage install's settings,
# models and start-at-login unit carry over.
export XDG_CONFIG_HOME="$HOME/.config" XDG_DATA_HOME="$HOME/.local/share" XDG_STATE_HOME="$HOME/.local/state"
# The sandbox's /tmp is private; host tools (maim, grim) write captures where both sides see the same path.
export TMPDIR="${XDG_CACHE_HOME:-$HOME/.cache}/tmp"
mkdir -p "$TMPDIR"
# Electron names its Wayland app_id / WM_CLASS desktop file after this (the .deb's is visioncli.desktop).
export CHROME_DESKTOP=com.thethirdsourcers.VisionCLI.desktop

if [ "$(basename "$0")" = visioncli-mcp ]; then
  export VISIONCLI_APP=/app/VisionCLI/visioncli
  exec /app/VisionCLI/resources/bin/visioncli-mcp "$@"
fi
# zypak (from the Electron base app) gives Chromium its sandbox inside Flatpak's.
exec zypak-wrapper /app/VisionCLI/visioncli.bin "$@"
