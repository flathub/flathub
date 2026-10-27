#!/bin/sh
# Chromium needs a writable TMPDIR inside the sandbox; zypak provides the Chromium sandbox.
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
exec zypak-wrapper /app/lfm-app/lfm-app --ozone-platform-hint=auto "$@"
