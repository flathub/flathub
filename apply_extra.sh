#!/bin/sh
# Généré par desktop/scripts/package-manifests.mjs — ne pas modifier à la main.
set -e
bsdtar -Oxf reader.deb 'data.tar*' | bsdtar -xf - --strip-components=3 './opt/Certyneo Reader'
rm -f reader.deb chrome-sandbox
