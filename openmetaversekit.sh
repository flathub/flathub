#!/bin/sh
set -eu

export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
mkdir -p "${TMPDIR}"
export OPEN_METAVERSE_APP_URL="file:///app/share/openmetaversekit/dist/index.html"
export OPEN_METAVERSE_DESKTOP_NAME="com.matthew.openmetaversekit.desktop"

exec zypak-wrapper.sh /app/bin/electron /app/share/openmetaversekit "$@"
