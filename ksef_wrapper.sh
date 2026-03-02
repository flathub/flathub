#!/bin/sh
# Prostszy wrapper bez LD_LIBRARY_PATH
DATA_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/KsefInvoice"
mkdir -p "$DATA_DIR"
cd "$DATA_DIR"
exec /app/bin/KsefInvoice "$@"
