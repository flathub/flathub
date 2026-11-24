#!/bin/sh
# CBSChat Flatpak launcher. zypak-wrapper (from the Electron base app) gives Chromium its sandbox inside Flatpak.
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID:-com.cbsalgo.CBSChat}"
if [ -n "${WAYLAND_DISPLAY}" ]; then
  set -- --ozone-platform-hint=auto --enable-features=WaylandWindowDecorations "$@"
fi
exec zypak-wrapper /app/cbschat/cbschat "$@"
