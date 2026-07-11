#!/bin/sh
# DLSS Updater launcher (Flathub build)
export LD_LIBRARY_PATH="/app/lib64:/app/lib:$LD_LIBRARY_PATH"
# Point flet at the client bundled in this flatpak (resolution order:
# bundled -> FLET_VIEW_PATH -> ~/.flet download; the download path never
# triggers because this is always set).
export FLET_VIEW_PATH=/app/share/flet-client
exec /app/bin/python3.14t /app/share/dlss-updater/main.py "$@"
