#!/bin/bash
# OpenROM Flatpak launcher
# openrom-core is on PATH via /app/bin — Flutter app calls it directly
export PATH="/app/bin:$PATH"
exec /app/gui "$@"
