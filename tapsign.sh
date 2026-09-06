#!/bin/sh
export TAPSIGN_ROOT="${TAPSIGN_ROOT:-/app/extra/share/tapsign}"
exec /app/extra/bin/tapsign "$@"
