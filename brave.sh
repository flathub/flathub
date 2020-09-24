#!/usr/bin/bash
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
which zypak-wrapper    2>/dev/null >/dev/null && wrapper="zypak-wrapper"
which zypak-wrapper.sh 2>/dev/null >/dev/null && wrapper="zypak-wrapper.sh"
exec $wrapper /app/extra/brave "$@"
