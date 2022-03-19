#!/usr/bin/env sh

set -eux

install -Dm0755 apply_extra.sh ${FLATPAK_DEST}/bin/apply_extra
install -Dm0755 xclicker.sh "${FLATPAK_DEST}/bin/xclicker-start"

bsdtar -Oxf xclicker.deb 'data.tar.*' |
  bsdtar -xf - \
    --strip-components=3 \
    --exclude='./usr/bin/' \

install -Dm0644 applications/xclicker.desktop "${FLATPAK_DEST}/share/applications/${FLATPAK_ID}.desktop"

desktop-file-edit --set-key="Icon" --set-value="${FLATPAK_ID}" "${FLATPAK_DEST}/share/applications/${FLATPAK_ID}.desktop"

# install -Dm0644 pixmaps/xclicker.png ${FLATPAK_DEST}/share/pixmaps/${FLATPAK_ID}.png
for size in 64 128 264 512; do
    convert pixmaps/xclicker.png -resize ${size} "${FLATPAK_ID}.png"
    install -Dm0644 "${FLATPAK_ID}.png" "${FLATPAK_DEST}/share/icons/hicolor/${size}x${size}/apps/${FLATPAK_ID}.png"
done
