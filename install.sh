#!/bin/bash
set -e

VERSION=$(grep 'PACKAGE_VERSION' electroncash/version.py | cut -d"'" -f2)
DATE=$(date -u +%Y-%m-%d)

sed -e "s/@VERSION@/$VERSION/g" -e "s/@DATE@/$DATE/g" \
    org.electroncash.ElectronCash.appdata.xml > /tmp/appdata.xml

install -Dm644 /tmp/appdata.xml ${FLATPAK_DEST}/share/metainfo/${FLATPAK_ID}.appdata.xml
install -Dm644 electron-cash.desktop ${FLATPAK_DEST}/share/applications/${FLATPAK_ID}.desktop
install -Dm644 icons/electron-cash.png ${FLATPAK_DEST}/share/icons/hicolor/256x256/apps/${FLATPAK_ID}.png
install -Dm644 icons/electron-cash.svg ${FLATPAK_DEST}/share/icons/hicolor/scalable/apps/${FLATPAK_ID}.svg
desktop-file-edit --set-key="Icon" --set-value=${FLATPAK_ID} ${FLATPAK_DEST}/share/applications/${FLATPAK_ID}.desktop
install -Dm755 ${FLATPAK_DEST}/bin/electron-cash ${FLATPAK_DEST}/bin/electron-cash.real
install -Dm755 electron-cash.sh ${FLATPAK_DEST}/bin/electron-cash
install -Dm644 LICENCE ${FLATPAK_DEST}/share/licenses/${FLATPAK_ID}/LICENCE
