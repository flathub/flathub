#!/usr/bin/env bash

# Copyright (c) 2023 Álan Crístoffer
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

YARNRC="$FLATPAK_BUILDER_BUILDDIR/yarnrc"
echo yarn-offline-mirror "\"$FLATPAK_BUILDER_BUILDDIR/flatpak-node/yarn-mirror\"" > "$FLATPAK_BUILDER_BUILDDIR/yarnrc"

# Install dependencies
mkdir -p desktop/www desktop/build
cp -r src/desktop/* desktop/www/

yarn install --offline --use-yarnrc "$YARNRC"
yarn install --offline --use-yarnrc "$YARNRC" --cwd Lachesis
yarn install --offline --use-yarnrc "$YARNRC" --cwd desktop
yarn install --offline --use-yarnrc "$YARNRC" --cwd desktop/www

pushd desktop/www || exit
npx tsc
rm -r node_modules index.ts tsconfig.json
yarn install --omit=dev --offline --use-yarnrc "$YARNRC"
popd || exit

pushd Lachesis || exit
npx ng build --configuration production --optimization
popd || exit

mv Lachesis/dist/Lachesis desktop/www/Lachesis
cp -r src/icons/* desktop/build/

pushd desktop || exit
npx electron-builder --dir --linux --publish never
popd || exit
