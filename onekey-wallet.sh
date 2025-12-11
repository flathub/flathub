#!/bin/sh

set -euo pipefail

APPIMAGE="/app/extra/OneKey-Wallet.AppImage"
export FLATPAK_ID="${FLATPAK_ID:-so.onekey.wallet}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
SESSION_TYPE="${XDG_SESSION_TYPE:-}"

# Wayland support can be toggled via USE_WAYLAND=1
USE_WAYLAND="${USE_WAYLAND:-0}"
EXIT_CODE=0

[ -x "${APPIMAGE}" ] || chmod +x "${APPIMAGE}"

if [ "${USE_WAYLAND}" -eq 1 ] && [ "${SESSION_TYPE}" = "wayland" ]; then
    zypak-wrapper "${APPIMAGE}" --appimage-extract-and-run \
      --enable-features=UseOzonePlatform,WaylandWindowDecorations \
      --ozone-platform=wayland "$@" || EXIT_CODE=$?
    # Fallback to X11 if Wayland launch fails with 133
    [ "${EXIT_CODE}" -ne 133 ] && exit "${EXIT_CODE}"
fi

exec zypak-wrapper "${APPIMAGE}" --appimage-extract-and-run "$@"
