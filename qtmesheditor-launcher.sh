#!/bin/sh
# Flatpak entrypoint. The release .deb already bundles Qt/Ogre under
# share/qtmesheditor + lib/qtmesheditor; remap those paths under $FLATPAK_DEST (/app).
set -eu

APP="${FLATPAK_DEST:-/app}"
export LD_LIBRARY_PATH="${APP}/lib/qtmesheditor:${APP}/share/qtmesheditor:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${APP}/lib/qtmesheditor/plugins:${APP}/share/qtmesheditor${QT_PLUGIN_PATH:+:${QT_PLUGIN_PATH}}"
export QML2_IMPORT_PATH="${APP}/share/qtmesheditor/qml${QML2_IMPORT_PATH:+:${QML2_IMPORT_PATH}}"
# Ogre needs X11 window handles; prefer XCB when the host is Wayland.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export OGRE_PLUGIN_DIR="${APP}/lib/qtmesheditor"

exec "${APP}/share/qtmesheditor/qtmesheditor" "$@"
