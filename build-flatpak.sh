#!/bin/bash

echo "Don't use this until this issue is fixed: https://github.com/flatpak/flatpak-builder-tools/issues/381"
exit

cd source/
npm install --offline

cd app/
npm install --offline
npx tsc

cd app/
npm install --offline
npm run build

cd ../../
npm run package:linux
