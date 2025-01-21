#!/bin/bash

EXTRA_ARGS+=(
  "--no-sandbox"
  "--ozone-platform-hint=auto"
  "--disable-seccomp-filter-sandbox"
)

export PTYSHELL=/bin/bash
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper "/app/Freelens/freelens" "${EXTRA_ARGS[@]}" "$@"
