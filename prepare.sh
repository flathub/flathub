#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
UPSTREAM_DIR="/tmp/stoatchat/for-desktop"

if ! command -v flatpak-node-generator &>/dev/null; then
    echo "Installing flatpak-node-generator via pipx..."
    # Make sure ~/.local/bin is on PATH for this session
    export PATH="${HOME}/.local/bin:${PATH}"
    pipx install flatpak-node-generator
    if ! command -v flatpak-node-generator &>/dev/null; then
        echo "ERROR: flatpak-node-generator still not found after install."
        exit 1
    fi
fi


if [ ! -d "$UPSTREAM_DIR" ] ; then
    git clone https://github.com/stoatchat/for-desktop.git --depth=1 --branch=v1.1.12 "$UPSTREAM_DIR"
fi

cd "$UPSTREAM_DIR"

npm install --package-lock-only --lockfile-version 1
pnpm make

cd "$SCRIPT_DIR"

flatpak-node-generator --electron-node-headers npm "$UPSTREAM_DIR/package-lock.json" -o generated-sources.json
# Replace the following line:
# "ln -sf \"linux-x64@0.21.5\" \"bin/esbuild-current\""
# With this line:
# "ln -sf \"@esbuild/linux-x64@0.21.5\" \"bin/esbuild-current\""

cp "$UPSTREAM_DIR/package-lock.json" "$SCRIPT_DIR/package-lock.json"
cp -r "$UPSTREAM_DIR/.vite/" "$SCRIPT_DIR/.vite/"
