#!/bin/sh

set -oue pipefail

export FLATPAK_ID="${FLATPAK_ID:-org.dbgate.DbGate}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/dbgate/dbgate $@
