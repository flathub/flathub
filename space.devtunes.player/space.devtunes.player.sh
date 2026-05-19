#!/bin/bash
# Enable Electron sandbox wrapper inside Flatpak
exec zypak-wrapper /app/main/dev-tunes "$@"
