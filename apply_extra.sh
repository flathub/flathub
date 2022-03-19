#!/usr/bin/env sh

set -e

bsdtar -Oxf xclicker.deb "data.tar.*" | tar -xJf -

rm -rf xclicker.deb
