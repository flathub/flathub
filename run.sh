#!/bin/sh

# TabTube launch wrapper (mirrors FreeTube's Flathub wrapper).
# zypak-wrapper provides Chromium's sandbox under Flatpak; TMPDIR must be a
# writable, app-private path inside the sandbox.
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
exec zypak-wrapper /app/tabtube/tabtube "$@"
