#!/bin/sh
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
mkdir -p "$TMPDIR"
exec zypak-wrapper /app/extra/ledger-live/ledger-live-desktop \
  --no-sandbox \
  --ozone-platform-hint=auto \
  "$@"
