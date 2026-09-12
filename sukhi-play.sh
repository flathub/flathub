#!/bin/sh
# Electron's own sandbox helper cannot be setuid inside a flatpak, so Electron
# is launched through zypak, which the Electron base app provides.
exec zypak-wrapper /app/lib/sukhi-play/electron \
  /app/lib/sukhi-play/resources/app "$@"
