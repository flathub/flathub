#!/bin/bash
set -e

if [ "$FLATPAK_ARCH" == "x86_64" ]; then
  # Use Intel BCD library, which is more accurate
  BINARY=plus42dec
  BUILD_ARGS="BCD_MATH=1"
else
  # Use floating point on other platforms
  BINARY=plus42bin
  BUILD_ARGS=""
fi

(cd gtk && make -j"${FLATPAK_BUILDER_N_JOBS}" AUDIO_ALSA=1 ${BUILD_ARGS})

install -Dm755 -s gtk/${BINARY} "${FLATPAK_DEST}/bin/${BINARY}"
(cd "${FLATPAK_DEST}/bin" && ln -s ${BINARY} plus42)

install -Dm644 gtk/icon-48x48.png "${FLATPAK_DEST}/share/icons/hicolor/48x48/apps/${FLATPAK_ID}.png"
install -Dm644 gtk/icon-128x128.png "${FLATPAK_DEST}/share/icons/hicolor/128x128/apps/${FLATPAK_ID}.png"
install -Dm644 "${FLATPAK_ID}.desktop" "${FLATPAK_DEST}/share/applications/${FLATPAK_ID}.desktop"
install -Dm644 "${FLATPAK_ID}.metainfo.xml" "${FLATPAK_DEST}/share/metainfo/${FLATPAK_ID}.metainfo.xml"
