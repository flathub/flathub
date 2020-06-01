#!/bin/bash
set -e
SRCDIR="$1"
DATADIR="$2"

function get_field() {
    jq -r ".$1" "${SRCDIR}/product.json"
}

NAME=$(get_field applicationName)
NAME_SHORT=$(get_field nameShort)
NAME_LONG=$(get_field nameLong)
EXEC=$NAME
ICON=$NAME
URLPROTOCOL=$(get_field urlProtocol)
LICENSE=$(get_field licenseName)

mkdir -p "${DATADIR}/applications"
sed \
  -e "s/@@NAME_LONG@@/${NAME_LONG}/g" \
  -e "s/@@NAME_SHORT@@/${NAME_SHORT}/g" \
  -e "s/@@EXEC@@/${EXEC}/g" \
  -e "s/@@ICON@@/${ICON}/g" \
  "${SRCDIR}/resources/linux/code.desktop" > "${DATADIR}/applications/${NAME}.desktop"
sed \
  -e "s/@@NAME_LONG@@/${NAME_LONG}/g" \
  -e "s/@@EXEC@@/${EXEC}/g" \
  -e "s/@@ICON@@/${ICON}/g" \
  -e "s/@@URLPROTOCOL@@/${URLPROTOCOL}/g" \
  "${SRCDIR}/resources/linux/code-url-handler.desktop" > "${DATADIR}/applications/${FLATPAK_ID}-url-handler.desktop"

mkdir -p "${DATADIR}/appdata"
sed \
  -e "s/@@NAME@@/${NAME}/g" \
  -e "s/@@NAME_LONG@@/${NAME_LONG}/g" \
  -e "s/@@LICENSE@@/${LICENSE}/g" \
  "${SRCDIR}/resources/linux/code.appdata.xml" > "${DATADIR}/appdata/${NAME}.appdata.xml"

for s in 32 64 128 256 512; do
  dest="${DATADIR}/icons/hicolor/${s}x${s}/apps"
  mkdir -p "${dest}"
  magick convert "${SRCDIR}/resources/linux/code.png" -resize ${s}x${s} "${dest}/${ICON}.png"
done
