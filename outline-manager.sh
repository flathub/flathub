#!/bin/sh

set -oue pipefail

export FLATPAK_ID="${FLATPAK_ID:-org.getoutline.OutlineManager}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/outline-manager/outline-manager $@
