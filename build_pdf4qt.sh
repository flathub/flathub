#!/bin/bash
set -e

export VCPKG_ROOT=$(pwd)/vcpkg

${VCPKG_ROOT}/vcpkg install --no-downloads tbb openssl lcms zlib openjpeg freetype ijg-libjpeg libpng 

# Build
cd pdf4qt
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DPDF4QT_INSTALL_QT_DEPENDENCIES=0 -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=/app
cmake --build build
cmake --install build

# Fix placement of plugins
cd ..

mkdir -p /app/lib/pdf4qt
mv -f /app/lib/*Plugin.so*  /app/lib/pdf4qt/

