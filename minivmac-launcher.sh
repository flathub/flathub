#!/usr/bin/env bash
set -euo pipefail

if [[ -n "${MINIVMAC_LIBEXEC_DIR:-}" ]]; then
  libexec_dir="${MINIVMAC_LIBEXEC_DIR}"
elif [[ -x /app/libexec/minivmac/minivmac-wayland ]]; then
  libexec_dir="/app/libexec/minivmac"
else
  libexec_dir="/usr/libexec/minivmac"
fi

wayland_bin="${libexec_dir}/minivmac-wayland"
x11_bin="${libexec_dir}/minivmac-x11"
backend="${MINIVMAC_BACKEND:-auto}"

case "${backend}" in
  wayland)
    exec env SDL_VIDEODRIVER=wayland "${wayland_bin}" "$@"
    ;;
  x11)
    exec "${x11_bin}" "$@"
    ;;
  auto|"")
    if [[ "${XDG_SESSION_TYPE:-}" == "wayland" || -n "${WAYLAND_DISPLAY:-}" ]]; then
      exec env SDL_VIDEODRIVER=wayland "${wayland_bin}" "$@"
    fi
    exec "${x11_bin}" "$@"
    ;;
  *)
    echo "Invalid MINIVMAC_BACKEND: ${backend} (expected auto, wayland, or x11)" >&2
    exit 2
    ;;
esac
