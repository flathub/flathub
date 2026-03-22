#!/usr/bin/env bash
export VLC_AOUT="pulse"

mkdir -p "${XDG_DATA_HOME:-$HOME/.local/share}/applications"
exec /app/bin/root-app/usr/bin/Root "$@"