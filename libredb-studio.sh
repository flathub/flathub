#!/bin/sh
# Flatpak entry point for LibreDB Studio (issue #232).
#
# The desktop shell is a plain WebKitGTK binary, so unlike the Electron apps this
# packaging pattern comes from there is no sandbox-relaunch wrapper to invoke and
# no library path to fix up: the GNOME runtime provides WebKitGTK, GTK and their
# helper processes. The shell finds its own sidecar (usr/bin/node) and the server
# payload (usr/lib/LibreDB Studio/payload) relative to this binary.
#
# The server writes its SQLite storage and generated credentials under
# $XDG_DATA_HOME, which Flatpak points at ~/.var/app/org.libredb.Studio/data.
set -eu

exec /app/libredb-studio/usr/bin/libredb-studio-desktop "$@"
