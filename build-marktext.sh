#!/bin/bash
set -e

APPIMAGE_NAME="marktext_x86_64.AppImage"
APPIMAGE_UPDATE_FILE="squashfs-root/app/resources/app-update.yml"

[ "$FLATPAK_ARCH" != "x86_64" ] && echo "[ERROR] Invalid architecture!" && exit 1;
[ ! -f "$APPIMAGE_NAME" ] && echo "[ERROR] Cannot find binary!" && exit 1;

if [ -d "squashfs-root" ]; then
    rm -rf squashfs-root
fi

mkdir -p /app/marktext

./${APPIMAGE_NAME} --appimage-extract

if [ -f "$APPIMAGE_UPDATE_FILE" ]; then
    rm -f "$APPIMAGE_UPDATE_FILE"
fi

mv squashfs-root/app/* /app/marktext
rm -rf squashfs-root

ln -sf /app/marktext/marktext /app/bin/marktext

exit 0
