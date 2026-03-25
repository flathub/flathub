#!/usr/bin/env bash
export VLC_AOUT="pulse"
mkdir -p /tmp/.rootapp-local
exec /app/extra/root-app/usr/bin/Root "$@"