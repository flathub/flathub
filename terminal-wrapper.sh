#!/bin/bash

# Terminal wrapper for flatpak - uses host system terminal commands

# Try different terminal emulators in order of preference
for terminal in kgx gnome-terminal xterm konsole alacritty; do
    if command -v "$terminal" >/dev/null 2>&1; then
        exec flatpak-spawn --host "$terminal" "$@"
    fi
done