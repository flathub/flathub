#!/bin/sh

set -oue pipefail

export FLATPAK_ID="${FLATPAK_ID:-io.beekeeperstudio.Studio}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/beekeeper-studio/beekeeper-studio-bin $@
