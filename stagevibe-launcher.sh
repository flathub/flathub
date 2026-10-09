#!/bin/sh
# Flatpak launcher. Wrap the Electron binary with zypak so Chromium's
# sandbox works inside the Flatpak sandbox.
exec zypak-wrapper /app/stagevibe/stagevibe "$@"
