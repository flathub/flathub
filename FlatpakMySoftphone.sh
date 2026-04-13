#!/usr/bin/env bash
set -e

APPDIR="/app/extra"

export LD_LIBRARY_PATH="/app/lib:/app/lib64:${APPDIR}/lib:${LD_LIBRARY_PATH:-}"

exec "${APPDIR}/MySoftphone" "$@"