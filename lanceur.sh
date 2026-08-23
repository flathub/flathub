#!/bin/sh
# Dans le bac a sable de Flatpak, le bac a sable interne de Chromium fait doublon
# et empeche le demarrage : on le desactive.
exec /app/sonora/sonora --no-sandbox "$@"
