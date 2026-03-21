#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
UPSTREAM_DIR="/tmp/stoatchat/for-desktop"

if ! command -v flatpak-node-generator &>/dev/null; then
    echo "Installing flatpak-node-generator via pipx..."
    # Make sure ~/.local/bin is on PATH for this session
    export PATH="${HOME}/.local/bin:${PATH}"
    #TODO - replace with url when pnpm implemented
    pipx install git+https://github.com/flatpak/flatpak-builder-tools.git#subdirectory=node --force
    if ! command -v flatpak-node-generator &>/dev/null; then
        echo "ERROR: flatpak-node-generator still not found after install."
        exit 1
    fi
fi

if [ ! -d "$UPSTREAM_DIR" ] ; then
    git clone https://github.com/stoatchat/for-desktop.git --depth=1 --branch=v1.3.0 "$UPSTREAM_DIR"
fi

cd "$UPSTREAM_DIR"

pnpm install

cd "$SCRIPT_DIR"

flatpak-node-generator pnpm "$UPSTREAM_DIR/pnpm-lock.yaml" -o generated-sources.json --electron-node-headers
