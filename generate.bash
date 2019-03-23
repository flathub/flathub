#!/bin/bash

set -eu

TAG=tutanota-release-3.50.1

export cache="${PWD}/generate-cache"
rm -rf "${cache}"
mkdir -p "${cache}"

[ -d tutanota ] || git clone https://github.com/tutao/tutanota.git
cd tutanota
git checkout -f "${TAG}"
patch -p1 <../generate.patch
python3 ../flatpak-npm-generator.py package-lock.json -o ../generated-sources.json
npm install
export npm_config_cache="${cache}"
node dist release -l
python3 ../flatpak-npm-generator.py build/dist/package-lock.json -o ../dist-generated-sources.json
cp build/dist/package-lock.json ..
