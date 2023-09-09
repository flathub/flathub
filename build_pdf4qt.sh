#!/bin/bash
set -e

# Install VCPKG
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh -disableMetrics
cd ..
export VCPKG_ROOT=$(pwd)/vcpkg

# Install tbb
${VCPKG_ROOT}/vcpkg install tbb

# Build
cd pdf4qt
export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DPDF4QT_INSTALL_QT_DEPENDENCIES=0 -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=/app
cmake --build build
cmake --install build

# Move icons and desktop files to the right place
cd ..

mkdir -p /app/lib/pdf4qt
mkdir -p /app/share/icons
mkdir -p /app/share/applications
mkdir -p /app/share/metainfo

mv -f /app/lib/*Plugin.so*  /app/lib/pdf4qt/
mv -f /app/usr/share/icons/*.svg /app/share/icons/
mv -f /app/usr/share/applications/*.desktop /app/share/applications/
mv -f .appdata.xml /app/share/metainfo/

rmdir /app/usr/share/icons
rmdir /app/usr/share/applications
rmdir /app/usr/share
rmdir /app/usr

sed -i 's|/usr/|/app/|g' /app/share/applications/*.desktop
