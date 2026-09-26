#!/bin/sh
# Généré par desktop/scripts/package-manifests.mjs — ne pas modifier à la main.
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
export CHROME_DESKTOP=com.certyneo.Reader.desktop
exec zypak-wrapper /app/extra/certyneo-reader-desktop --ozone-platform-hint=auto "$@"
