#!/bin/bash

WAYLAND_FLAGS=""
if [[ -n "${WAYLAND_DISPLAY}" ]]; then
fi

export ELECTRON_TRASH=gio
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

mkdir -p "${TMPDIR}"

exec /app/antigravity/antigravity \
    --no-sandbox \
