#!/bin/sh
# Launcher for the PIA Electron application inside the Flatpak sandbox.
# zypak-wrapper is provided by org.electronjs.Electron2.BaseApp and lets the
# bundled Chromium use the Flatpak sandbox instead of its own setuid sandbox.
exec zypak-wrapper /app/main/pia "$@"

