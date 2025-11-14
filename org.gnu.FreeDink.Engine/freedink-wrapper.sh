#!/bin/sh
# Wrapper for org.gnu.FreeDink.Engine Flatpak
# - Keeps the free GNU FreeDink data in /app (read-only)
# - Lets the user put original Dink / D-Mod data in XDG_DATA_HOME or ~/.local/share/dink

set -eu

ENGINE_BIN="/app/bin/freedink"

# -------------------------------
# Resolve a user data directory
# -------------------------------
if [ -n "${XDG_DATA_HOME:-}" ]; then
    USER_DATA_ROOT="$XDG_DATA_HOME"
else
    USER_DATA_ROOT="$HOME/.local/share"
fi

USER_DINK_DIR="$USER_DATA_ROOT/dink"

# Ensure the user dir exists so configs / saves / mods can live there
mkdir -p "$USER_DINK_DIR"

# -------------------------------
# Choose a working directory
# -------------------------------
# Priority:
#  1. A "dink" or "dmods" directory in USER_DINK_DIR
#  2. Current directory if it looks like a Dink dir
#  3. USER_DINK_DIR as a safe default
if [ -d "$USER_DINK_DIR/dink" ] || [ -d "$USER_DINK_DIR/dmods" ]; then
    cd "$USER_DINK_DIR"
elif [ -d "$PWD/dink" ] || [ -d "$PWD/dmods" ]; then
    :
else
    cd "$USER_DINK_DIR"
fi

# -------------------------------
# Launch the engine
# -------------------------------
exec "$ENGINE_BIN" "$@"
