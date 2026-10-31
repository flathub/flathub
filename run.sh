#!/bin/sh
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
export DOTNET_CLI_TELEMETRY_OPTOUT=1
exec zypak-wrapper /app/main/trading-platform --disable-dev-shm-usage "$@"
