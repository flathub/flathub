#!/bin/sh
# Flatpak launcher: the real binary is unpacked from the .deb into
# /app/extra by apply_extra at install time.
export OXITIDE_DATA_DIR=/app/extra/usr/share/oxitide
exec /app/extra/usr/bin/oxitide "$@"
