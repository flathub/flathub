#!/bin/sh
set -eu

# extra-data unpacks into /app/extra; the Electron tree is extra/main.
# zypak-wrapper keeps Chromium's sandbox (do not pass --no-sandbox).
unset ELECTRON_RUN_AS_NODE
export TMPDIR="${XDG_CACHE_HOME:-/tmp}"
exec zypak-wrapper /app/extra/main/enotespace --ozone-platform-hint=auto "$@"
