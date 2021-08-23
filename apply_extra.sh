#!/bin/bash

set -o errexit
set -o pipefail
set -o nounset

FLATPAK_ID=gg.guilded.Guilded

mkdir -p export/share deb-package

ar p Guilded-Linux.deb data.tar.xz | tar -xJf - -C deb-package

mv deb-package/opt/Guilded guilded
mv deb-package/usr/share/{icons,applications} export/share/

rename "guilded" "${FLATPAK_ID}" export/share/{icons/hicolor/*/apps,applications}/guilded.*
desktop-file-edit --set-key="Exec" --set-value="guilded %U" \
	--set-key="Icon" --set-value="${FLATPAK_ID}" \
	--set-key="X-Flatpak-RenamedFrom" --set-value="guilded.desktop;" \
	"export/share/applications/${FLATPAK_ID}.desktop"

rm -r Guilded-Linux.deb deb-package
