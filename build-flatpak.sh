#!/bin/bash

cd source/
npm install --offline

cd app/
npm install --offline

cd ../

npm run build
npm run smui-theme
npx electron-builder --dir --linux --publish never
