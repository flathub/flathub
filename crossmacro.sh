#!/bin/sh
# CrossMacro Flatpak Launcher
# Uses dotnet runtime installed by install.sh

export DOTNET_ROOT=/app/lib/dotnet
exec /app/lib/dotnet/dotnet /app/lib/crossmacro/CrossMacro.UI.dll "$@"
