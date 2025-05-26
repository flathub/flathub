#!/bin/bash

# Detect the architecture

ARCH_RAW=$(uname -m)

case "$ARCH_RAW" in
    x86_64)
        ARCH="amd64"
        ;;
    aarch64)
        ARCH="aarch64"
        ;;
    *)
        ARCH="$ARCH_RAW"
        ;;
esac


mkdir -p /app/lib/LogoRRR/

if [ "$ARCH" == "amd64" ]; then
    echo "Building for x86_64 architecture"
    cp -ar $ARCH/binaries/LogoRRR/* /app/lib/LogoRRR/
elif [ "$ARCH" == "aarch64" ]; then
    echo "Building for aarch64 architecture"
    cp -ar $ARCH/binaries/LogoRRR/* /app/lib/LogoRRR/
else
    echo "Unsupported architecture: $ARCH"
    exit 1
fi

mkdir -p /app/bin
ln -s /app/lib/LogoRRR/bin/LogoRRR /app/bin/LogoRRR
install -Dm644 $ARCH/icons/logorrr-icon-16.png /app/share/icons/hicolor/16x16/apps/app.logorrr.LogoRRR.png
install -Dm644 $ARCH/icons/logorrr-icon-32.png /app/share/icons/hicolor/32x32/apps/app.logorrr.LogoRRR.png
install -Dm644 $ARCH/icons/logorrr-icon-64.png /app/share/icons/hicolor/64x64/apps/app.logorrr.LogoRRR.png
install -Dm644 $ARCH/icons/logorrr-icon-128.png /app/share/icons/hicolor/128x128/apps/app.logorrr.LogoRRR.png
install -Dm644 $ARCH/icons/logorrr-icon-256.png /app/share/icons/hicolor/256x256/apps/app.logorrr.LogoRRR.png
install -Dm644 $ARCH/icons/logorrr-icon-512.png /app/share/icons/hicolor/512x512/apps/app.logorrr.LogoRRR.png
install -Dm644 $ARCH/meta/app.logorrr.LogoRRR.metainfo.xml /app/share/metainfo/app.logorrr.LogoRRR.metainfo.xml
install -Dm644 $ARCH/meta/app.logorrr.LogoRRR.desktop -t /app/share/applications
