#!/bin/sh
# Flatpak entry point: provision a writable data disk from the read-only
# template shipped at /app/bin/qemu/data.img, then hand off to the QEMU
# launcher with QEMU_DATA_IMG pointing at the writable copy.
set -e

DATA_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/wrapper-lite"
mkdir -p "$DATA_DIR"

DATA_IMG="$DATA_DIR/data.img"
if [ ! -f "$DATA_IMG" ]; then
    cp /app/bin/qemu/data.img "$DATA_IMG" 2>/dev/null || true
fi
chmod 0644 "$DATA_IMG" 2>/dev/null || true

export QEMU_DATA_IMG="$DATA_IMG"
exec /app/bin/wrapper-lite-qemu "$@"