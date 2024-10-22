#!/bin/bash

set -euo pipefail

BIN_NAME="rune"
PACKAGE_NAME="ci.not.Rune"

SOURCE_APP="$(pwd)/release"
SOURCE_APPSTREAM="$(pwd)/tools/$PACKAGE_NAME.metainfo.xml"
SOURCE_DESKTOP_FILE="$(pwd)/tools/$PACKAGE_NAME.desktop"
SOURCE_ICONS="$(pwd)/source/assets/icons/Papirus"

TARGET_PREFIX="/app"
TARGET_APP="$TARGET_PREFIX/$BIN_NAME"
TARGET_APPSTREAM="$TARGET_PREFIX/share/metainfo"
TARGET_BIN="$TARGET_PREFIX/bin"
TARGET_DESKTOP_FILE="$TARGET_PREFIX/share/applications"
TARGET_ICONS="$TARGET_PREFIX/share/icons/hicolor"

echo "Installing binary ..."
mkdir -pv "$TARGET_APP"
cp -rv "$SOURCE_APP"/* "$TARGET_APP"
chmod +x "$TARGET_APP/$BIN_NAME"

mkdir -pv "${TARGET_BIN}"
ln -sv "$TARGET_APP/$BIN_NAME" "$TARGET_BIN/$BIN_NAME"

echo "Installing icons ..."
mkdir -p "$TARGET_ICONS"
for i in "16x16" "32x32" "64x64" "128x128" "256x256" "512x512"; do
	mkdir -p "$TARGET_ICONS/$i/apps"
	install -Dvm644 "$SOURCE_ICONS/$i/apps/$BIN_NAME.png" \
 		"$TARGET_ICONS/$i/apps/$PACKAGE_NAME.png"
done

echo "Installing desktop icon ..."
mkdir -pv "$TARGET_DESKTOP_FILE"
install -Dvm644 "$SOURCE_DESKTOP_FILE" "$TARGET_DESKTOP_FILE"

echo "Installing AppStream definition ..."
mkdir -pv "$TARGET_APPSTREAM"
install -Dvm644 "$SOURCE_APPSTREAM" "$TARGET_APPSTREAM"
