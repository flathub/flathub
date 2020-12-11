#!/bin/bash

set -xe

ELECTRON_VERSION=11.0.4
ELECTRON_PACKAGER_VERSION=15.1.0
ASAR_VERSION=3.0.3

ELECTRON=electron@${ELECTRON_VERSION}
ELECTRON_PACKAGER=electron-packager@${ELECTRON_PACKAGER_VERSION}
ASAR=asar@${ASAR_VERSION}

FLATPAK_NODE_GENERATOR=$(pwd)/flatpak-node-generator.py

UNPACK=$(mktemp -d)

cp notion.exe $UNPACK/

pushd $UNPACK
7z x notion.exe
7z x \$PLUGINSDIR/app-64.7z
npx $ASAR extract resources/app.asar app

pushd app
for dep in $(npm --prefix shared/notion-intl ls --json 2>/dev/null|jq -r '.dependencies[].required._id'); do
  npm add --ignore-scripts "$dep"
done
npm add --save-dev --ignore-scripts $ELECTRON $ELECTRON_PACKAGER $ASAR
npm install --package-lock-only
npm audit fix
jq 'del(.dependencies["notion-intl"])' package-lock.json > package-lock.json~
mv package-lock.json{~,}
$FLATPAK_NODE_GENERATOR --electron-node-headers npm package-lock.json
popd

popd

cp $UNPACK/app/{package.json,package-lock.json,generated-sources.json} .
