#!/bin/sh
# zypak provides the Chromium sandbox via flatpak-spawn (no setuid helper in
# a flatpak), so the raw Electron binary is launched through it.
exec zypak-wrapper /app/extra/app/restorm-bin "$@"
