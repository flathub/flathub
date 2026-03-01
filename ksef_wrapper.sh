#!/bin/sh
export LD_LIBRARY_PATH=/app/lib/ksef:$LD_LIBRARY_PATH

# Ensure /app/share is in XDG_DATA_DIRS for theme discovery
if [ -z "$XDG_DATA_DIRS" ]; then
    export XDG_DATA_DIRS=/app/share:/usr/share
else
    # Prepend /app/share if not present (though Flatpak usually sets this)
    case "$XDG_DATA_DIRS" in
        */app/share*) ;;
        *) export XDG_DATA_DIRS=/app/share:"$XDG_DATA_DIRS" ;;
    esac
fi

# Ensure XDG_DATA_DIRS includes the path where themes are mounted
# Themes from extensions are usually in /app/share/themes
# But also check for user themes in ~/.local/share/themes which are mapped via filesystem permissions

# Force standard GTK theme path if needed
if [ -z "$GTK_PATH" ]; then
    export GTK_PATH=/app/lib/gtk-3.0:/usr/lib/gtk-3.0
fi

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

