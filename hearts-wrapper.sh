#!/bin/sh
# Hearts saves its settings to $HOME/.hearts. Inside the Flatpak sandbox the
# default $HOME is not persisted, so point it at the app's persistent per-app
# data directory ($XDG_DATA_HOME) — this makes settings survive across launches
# without any host filesystem access.
export HOME="$XDG_DATA_HOME"
exec /app/bin/hearts.bin "$@"
