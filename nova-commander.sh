#!/bin/sh
# zypak sets up Chromium's sandbox for Flatpak.
exec /app/lib/zypak/zypak-wrapper.sh /app/lib/nova-commander/nova-commander "$@"
