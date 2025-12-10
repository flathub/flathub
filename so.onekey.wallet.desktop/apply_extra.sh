#!/bin/sh
set -e
APP_IMAGE="/app/extra/OneKey-Wallet.AppImage"
if [ -f "$APP_IMAGE" ]; then
  chmod +x "$APP_IMAGE"
fi
exit 0
