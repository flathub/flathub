#!/bin/sh
set -eu

export TMPDIR="${XDG_RUNTIME_DIR:-/tmp}"
exec zypak-wrapper.sh /app/main/raffi "$@"
