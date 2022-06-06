#!/usr/bin/sh

set -e

bsdtar -Oxf freedownloadmanager.deb 'data.tar*' |
  bsdtar -xf - \
    --strip-components=4 \
    --exclude='./opt/freedownloadmanager/nacl*' \
    ./opt/freedownloadmanager
rm freedownloadmanager.deb
