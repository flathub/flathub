#!/usr/bin/env bash
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
exec /app/bin/SourceGit "$@"