#!/bin/sh

set -oue pipefail

export FLATPAK_ID="${FLATPAK_ID:-org.getoutline.OutlineClient}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/outline-client/outline-client $@
