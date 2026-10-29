#!/bin/sh
# Launcher (2026-10-29): the .deb's binary lives in /app/extra, unpacked by
# apply_extra. FLATPAK_ID tells it it is sandboxed (src-tauri/src/sandbox.rs).
exec /app/extra/usr/bin/codemany-desktop "$@"
