#!/bin/sh

export DOTNET_ROOT=/app/lib/dotnet

exec zypak-wrapper.sh /app/vrcx/vrcx --no-updater --no-install --no-desktop "$@"
