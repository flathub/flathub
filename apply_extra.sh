#!/usr/bin/sh
set -e

bsdtar -Oxf MAX.deb data.tar.xz |
  bsdtar -xf - \
    --strip-components=3 \
    ./opt/MAX
rm MAX.deb

mv MAX max
install -Dm755 /app/bin/stub_sandbox chrome-sandbox
