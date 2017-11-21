#!/usr/bin/env bash

# Build and install to a local prefix under this repository.
export OSTYPE

# Flags:

  # -g: install globally instead for all users
  # -s: link everything statically, no D-Bus communication. More likely to work!
  # -c: client to build
  # -p: number of processors to use

set -ex

make_install() {
    make install
}

TOP=`pwd`
INSTALL="/app"
BUILDDIR="build"

cd "${TOP}/daemon"
DAEMON="$(pwd)"
cd contrib
mkdir -p native
cd native
../bootstrap --prefix="/app"
make
cd "${DAEMON}"
./autogen.sh

./configure $sharedLib $CONFIGURE_FLAGS --prefix="/app"
make -j${proc}
make_install

cd "${TOP}/lrc"
mkdir -p ${BUILDDIR}
cd ${BUILDDIR}
cmake .. -DCMAKE_INSTALL_PREFIX="/app" -DCMAKE_BUILD_TYPE=Release $static
make -j${proc}
make_install

cd "${TOP}/${client}"
mkdir -p ${BUILDDIR}
cd ${BUILDDIR}
cmake .. -DCMAKE_INSTALL_PREFIX="/app" $static
make -j${proc}
make_install
