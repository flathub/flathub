#!/usr/bin/env sh

set -eux

install -Dm0755 src/xclicker "${FLATPAK_DEST}/bin/xclicker"

install -Dm0644 ../xclicker.desktop "${FLATPAK_DEST}/share/applications/${FLATPAK_ID}.desktop"

desktop-file-edit --set-key="Icon" --set-value="${FLATPAK_ID}" "${FLATPAK_DEST}/share/applications/${FLATPAK_ID}.desktop"

for size in 64 128 264 512; do
    convert ../img/icon.png -resize ${size} "${FLATPAK_ID}.png"
    install -Dm0644 "${FLATPAK_ID}.png" "${FLATPAK_DEST}/share/icons/hicolor/${size}x${size}/apps/${FLATPAK_ID}.png"
done
