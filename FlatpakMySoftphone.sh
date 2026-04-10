#!/usr/bin/env bash
set -e

APPDIR="/app/mysoftphone"

export LD_LIBRARY_PATH="/app/lib:${APPDIR}/lib:${LD_LIBRARY_PATH:-}"

exec "${APPDIR}/MySoftphone" "$@"
