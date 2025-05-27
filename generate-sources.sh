#!/bin/bash

git clone https://github.com/hperrin/stream-overlay.git
PIPX_BIN_DIR='bin' pipx install git+https://github.com/flatpak/flatpak-builder-tools.git#subdirectory=node

cd stream-overlay
git checkout "$(yq -r '.modules[0].sources[1].commit' ../com.hperrin.StreamOverlay.yml)"
cd ..

bin/flatpak-node-generator -o temp1.json npm stream-overlay/package-lock.json
bin/flatpak-node-generator -o temp2.json npm stream-overlay/app/package-lock.json
bin/flatpak-node-generator -o temp3.json npm stream-overlay/app/app/package-lock.json

jq -sc "flatten | unique | sort_by(.type)" temp1.json temp2.json temp3.json > generated-sources.json

rm -rf stream-overlay temp{1,2,3}.json
PIPX_BIN_DIR='bin' pipx uninstall flatpak-node-generator