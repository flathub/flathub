#!/bin/bash
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID:-io.github.Pak_Man926.FlixDesk}"
mkdir -p "${TMPDIR}"

# Execute Python FlixDesk
exec python3 /app/lib/flixdesk/flixdesk.py "$@"
