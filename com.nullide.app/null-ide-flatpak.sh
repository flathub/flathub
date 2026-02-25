#!/bin/bash

export ELECTRON_IS_DEV=0
export ELECTRON_FORCE_IS_PACKAGED=true
export NODE_ENV=production
export ELECTRON_ENABLE_LOGGING=1
export ELECTRON_ENABLE_STACK_DUMPING=1
export TMPDIR="${TMPDIR:-/var/tmp}"
export FLATPAK_ID=1

APP_DIR="/app/lib/null-ide"
ELECTRON="/app/lib/electron/electron"

if [ -x "/app/bin/zypak-wrapper" ]; then
    exec /app/bin/zypak-wrapper "$ELECTRON" "$APP_DIR" "$@"
else
    exec "$ELECTRON" "$APP_DIR" "$@"
fi
