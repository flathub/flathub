#!/bin/bash

export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec zypak-wrapper /app/main/facebook-messenger-desktop "$@"
