#!/bin/sh
# Launches the bundled Electron app inside the Flatpak sandbox. Uses the
# Electron BaseApp's zypak wrapper so Chromium's sandbox works under Flatpak.
exec zypak-wrapper "/app/sfmc-data-loader/sfmc-data-loader" "$@"
