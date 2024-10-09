#!/usr/bin/env sh

set -e

install -Dm755 apply_extra ${FLATPAK_DEST}/bin/apply_extra

bsdtar -Oxf sketcher.deb 'data.tar.*' |
  bsdtar -xf - \
    --strip-components=2 \
    --exclude='./etc/' \
    --exclude='./usr/bin/' \
    --exclude='./usr/share/doc/' \
    --exclude='./opt/WireframeSketcher/'

find share -type f -exec install -Dm644 "{}" "${FLATPAK_DEST}/{}" \;

install -Dm644 flatpak-dev-shim.jar ${FLATPAK_DEST}/flatpak-dev-shim.jar
install -Dm644 flatpak-dev-shim.so ${FLATPAK_DEST}/lib/libflatpakdevshim.so

ln -s ${FLATPAK_DEST}/extra/WireframeSketcher/WireframeSketcher ${FLATPAK_DEST}/bin/WireframeSketcher

desktop-file-edit --set-key=Exec --set-value=WireframeSketcher --set-key=TryExec --set-value=WireframeSketcher ${FLATPAK_DEST}/share/applications/wireframesketcherstudio.desktop 
