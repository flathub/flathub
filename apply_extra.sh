#!/bin/sh
# apply_extra: flatpak runs this once on the user's machine after it
# downloads the extra-data .deb (com.codemany.Codemany.yml), in /app/extra.
# 2026-10-29: unpack only the app binary; the desktop file, icon and
# metainfo are installed at build time under the app id instead.
set -eu
bsdtar -Oxf codemany-desktop.deb 'data.tar*' | bsdtar -xf - --include '*usr/bin/codemany-desktop'
rm -f codemany-desktop.deb
test -x usr/bin/codemany-desktop
