#!/bin/bash

# Chromium keeps its sockets under $TMPDIR, and the sandbox's /tmp is private
# per instance, so a second launch (a mailto: link) would not find the first.
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/skybox/skybox "$@"
