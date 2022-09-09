#!/usr/bin/env bash

set -euo pipefail

# global variables
FLATPAK_ID='__FLATPAK_ID__'

# unpack debian package
bsdtar -Oxf "Breitbandmessung-linux.deb" 'data.tar.*' |
    bsdtar -xf -

# /extra/usr/share => /extra/export/share
mkdir export
mv usr/share export/share
rm -rf usr

# .desktop file
mv export/share/applications/breitbandmessung.desktop "export/share/applications/$FLATPAK_ID.desktop"
sed -Ei 's|^Exec=.*|Exec=breitbandmessung|' "export/share/applications/$FLATPAK_ID.desktop"
sed -Ei "s|^Icon=.*|Icon=$FLATPAK_ID|" "export/share/applications/$FLATPAK_ID.desktop"

# rename icons and remove large sizes
find export/share/icons -name 'breitbandmessung.*' -path '*1024*' -delete
find export/share/icons -name 'breitbandmessung.*' -exec rename -v breitbandmessung "$FLATPAK_ID" {} \;

# clean up
rm Breitbandmessung-linux.deb
