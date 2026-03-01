#!/bin/sh
export LD_LIBRARY_PATH=/app/lib/ksef:$LD_LIBRARY_PATH

# Attempt theme integration
export QT_QPA_PLATFORMTHEME=gtk3

# Directory for database and user configuration - according to XDG
DATA_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/KsefInvoice"

if [ ! -d "$DATA_DIR" ]; then
    mkdir -p "$DATA_DIR"
fi # Added check for mkdir failure could also be good but simple mkdir -p is usually fine

# Change to working directory where the application can write files
if ! cd "$DATA_DIR"; then
    echo "Error: Failed to change directory to $DATA_DIR" >&2
    exit 1
fi

# Launch the application
exec /app/lib/ksef/KsefInvoice "$@"

