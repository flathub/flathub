#!/usr/bin/env bash
set -euo pipefail

echo "THIS IS MEANT TO RUN ON MY MACHINE DONT TOUCH IT DONT TOUCH IT"

flatpak run org.flatpak.Builder \
    --sandbox \
    --user \
    --install \
    --install-deps-from=flathub \
    --repo=".repo" \
    .build \
    io.github.relativemodder.fivetris.yml
