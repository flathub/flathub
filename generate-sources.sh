#!/bin/bash

git clone https://github.com/sciactive/quickdav.git
PIPX_BIN_DIR='bin' pipx install git+https://github.com/flatpak/flatpak-builder-tools.git#subdirectory=node

cd quickdav
git checkout "$(yq -r '.modules[0].sources[1].commit' ../com.sciactive.QuickDAV.yml)"
cd ..

bin/flatpak-node-generator -o temp1.json npm quickdav/package-lock.json
bin/flatpak-node-generator -o temp2.json npm quickdav/app/package-lock.json

jq -sc "flatten | unique | sort_by(.type)" temp1.json temp2.json > generated-sources.json

rm -rf quickdav temp{1,2}.json
PIPX_BIN_DIR='bin' pipx uninstall flatpak-node-generator