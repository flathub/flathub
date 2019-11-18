#!/usr/bin/env bash
set -e

FLATPAK_ID=com.avocode.Avocode

mkdir -p deb-package export/share

ar p avocode.deb data.tar.xz | tar -xJf - -C deb-package

mv deb-package/opt/Avocode avocode
mv deb-package/usr/share/{icons,applications} export/share/
rename "avocode" "${FLATPAK_ID}" export/share/{icons/hicolor/*/*,applications}/avocode.*
desktop-file-edit \
    --set-key="Exec" --set-value="avocode %U" \
    --set-key="Icon" --set-value="${FLATPAK_ID}" \
    --set-key="X-Flatpak-RenamedFrom" --set-value="avocode.desktop;" \
    "export/share/applications/${FLATPAK_ID}.desktop"

rm -r avocode.deb deb-package
