#!/bin/sh
# zypak-wrapper provides Chromium's sandbox inside the Flatpak sandbox (no setuid
# chrome-sandbox needed). The binary is placed at /app/extra/WooCommercePOS by
# apply_extra at install time.
exec zypak-wrapper /app/extra/WooCommercePOS "$@"
