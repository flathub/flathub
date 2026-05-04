#!/bin/sh
# Launcher wrapper for Deepink inside Flatpak.
#
# --no-sandbox       Electron's setuid chrome-sandbox is forbidden in Flatpak;
#                    the Flatpak sandbox itself provides the required isolation.
# --ozone-platform-hint=auto  Lets Electron 20+ choose Wayland when available,
#                    falling back to XWayland/X11 automatically.
exec /app/lib/deepink/deepink --no-sandbox --ozone-platform-hint=auto "$@"
