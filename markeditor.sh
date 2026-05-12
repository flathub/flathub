#!/bin/sh
# Flatpak launcher for MarkeDitor.
# The Avalonia binary lives next to its dependencies under /app/share/markeditor.
exec /app/share/markeditor/MarkeDitor "$@"
