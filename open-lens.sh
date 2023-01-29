#!/bin/sh

set -oue pipefail

export FLATPAK_ID="${FLATPAK_ID:-dev.k8slens.OpenLens}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/open-lens/open-lens "$@"
