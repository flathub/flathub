#!/bin/sh
set -e

if [ -z "$FLATPAK_TOOLS" ]; then
    echo "FLATPAK_TOOLS is not set"
    exit 1
fi

rm -rf swarmtunes-client

git clone --commit b2c60f4e722eaff2db88e7d46da05adc9fee0bb9 https://github.com/AceandGaming/swarmtunes-client.git
cd swarmtunes-client

python3 "$FLATPAK_TOOLS/cargo/flatpak-cargo-generator.py" \
    -o ../cargo-sources.json \
    src-tauri/Cargo.lock

flatpak-node-generator \
    --no-requests-cache \
    -o ../node-sources.json \
    npm package-lock.json

cd ..
rm -rf swarmtunes-client