#!/bin/bash
set -e

# Install VCPKG
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh -disableMetrics
cd ..
export VCPKG_ROOT=$(pwd)/vcpkg

# Instalace závislostí
${VCPKG_ROOT}/vcpkg install tbb

# Sestavení vaší aplikace
cd pdf4qt
export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DPDF4QT_INSTALL_QT_DEPENDENCIES=0 -DPDF4QT_LINK_TBB=ON -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=/app
cmake --build build
cmake --install build

mv -f /app/lib/*Plugin.so*  /app/lib/pdf4qt/
