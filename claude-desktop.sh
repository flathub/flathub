#!/bin/bash
# Launcher script for Claude Desktop Flatpak

export ELECTRON_OZONE_PLATFORM_HINT="${ELECTRON_OZONE_PLATFORM_HINT:-auto}"

# Use zypak for proper Flatpak sandbox integration with Electron
exec zypak-wrapper /app/lib/claude-desktop/electron /app/lib/claude-desktop/resources/app.asar "$@"
