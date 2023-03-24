#!/usr/bin/sh

set -e

bsdtar -Oxf edge.deb 'data.tar*' |
  bsdtar -xf - \
    --strip-components=4 \
    --exclude='./opt/microsoft/msedge-dev/nacl*' \
    ./opt/microsoft/msedge-dev
rm edge.deb

install -Dm755 /app/bin/stub_sandbox msedge-sandbox
