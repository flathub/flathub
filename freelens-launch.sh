#!/bin/bash

EXTRA_ARGS+=(
  "--ozone-platform-hint=auto"
)

export PTYSHELL=/bin/bash
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper "/app/Freelens/freelens" "${EXTRA_ARGS[@]}" "$@"
