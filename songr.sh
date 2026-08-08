#!/bin/sh
set -eu
APPDIR=/app/songr.AppDir
if [ ! -x "$APPDIR/AppRun" ]; then
  echo "Songr AppDir is missing under /app (missing AppRun)." >&2
  exit 1
fi
export APPDIR
exec "$APPDIR/AppRun" "$@"
