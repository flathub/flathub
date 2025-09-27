#!/bin/sh
APPIMAGE_PATH="/app/share/bitrip/BIT RIP-1_0_6-linux.AppImage"
chmod +x "$APPIMAGE_PATH"
# Evita depender de FUSE dentro del sandbox
export APPIMAGE_EXTRACT_AND_RUN=1
# Si tu AppImage requiere este flag, ya lo dejamos puesto:
exec "$APPIMAGE_PATH" --no-sandbox "$@"
