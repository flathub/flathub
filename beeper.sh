#!/bin/sh

set -oue pipefail

EXTRA_ARGS+=("--no-update")

export FLATPAK_ID="${FLATPAK_ID:-com.beeper.Beeper}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/beeper/beepertexts "${EXTRA_ARGS[@]}" "$@"
