#!/bin/bash

cd source/
npm install --offline

cd app/
npm install --offline
npx tsc

cd app/
npm install --offline
npm run build

cd ../../
npx electron-builder --dir --linux --publish never
