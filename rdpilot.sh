#!/bin/sh
# Launcher for the Flatpak build. The published output lives in /app/lib/rdpilot so that
# libfreerdp_wrapper.so stays beside RDPilot.Client.dll for default DllImport probing.
exec /app/lib/rdpilot/RDPilot.Client "$@"
