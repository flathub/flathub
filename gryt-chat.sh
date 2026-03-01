#!/bin/bash
export TMPDIR="$XDG_RUNTIME_DIR/app/${FLATPAK_ID:-com.gryt.Chat}"
exec zypak-wrapper /app/gryt-chat/gryt-chat \
  --ozone-platform-hint=auto \
  "$@"
