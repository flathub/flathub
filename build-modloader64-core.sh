#!/bin/bash
set -euo pipefail

mkdir -p /app/lib/modloader64/ModLoader-template

# Remove the async contributor fetch in prebuild so the build remains offline.
node -e 'let f=require("fs"),s=f.readFileSync("gulpfile.ts","utf8");f.writeFileSync("gulpfile.ts",s.replace(/\(async\s*\(\)\s*=>\s*\{[\s\S]*?\}\)\(\);/g,""));'

yarn config --offline set yarn-offline-mirror "$PWD/flatpak-node/yarn-mirror"
yarn install --frozen-lockfile --ignore-scripts --offline
npx patch-package
npx gulp _api
rm -rf node_modules/modloader64_api
ln -sf "$PWD/API/build" node_modules/modloader64_api
npx gulp emulator
npx gulp build_bin
mkdir -p build/emulator
cp -a Mupen64Plus/emulator/. build/emulator/
npx gulp prebuild
npx gulp _build
rm -rf node_modules/modloader64_api
cp -a API/build node_modules/modloader64_api
cp -a build/* /app/lib/modloader64/ModLoader-template/
cp -a package.json /app/lib/modloader64/ModLoader-template/
cp -a node_modules /app/lib/modloader64/ModLoader-template/
cp -a modloader64-config.json /app/lib/modloader64/ModLoader-template/ 2>/dev/null || true
cp -a modloader64-config.json /app/lib/modloader64/ModLoader-template/ModLoader64-config.json 2>/dev/null || true