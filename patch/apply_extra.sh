#!/bin/bash
set -e  # "panic" on error

cd /app/extra
bsdtar --to-stdout -xf cloudpc.deb data.* | bsdtar -xf -
mv opt/chuanyun-vdi-client .

APP_ROOT="chuanyun-vdi-client"
CCSDK_DIR="$APP_ROOT/resources/app.asar.unpacked/node_modules/chuanyunAddOn/ccsdk/uos"
CCSDK_ZTE_DIR="$APP_ROOT/resources/app.asar.unpacked/node_modules/chuanyunAddOn-zte/ccsdk"
LIB_DIR="$CCSDK_DIR/lib"
LOG_DIR="$CCSDK_DIR/log"

# Let app use the latest stable version provided by Flatpak instead of the outdated version that comes with it
rm -f "$LIB_DIR/libm.so.6" "$LIB_DIR/libmount.so.1" "$LIB_DIR/libc.so.6" "$LIB_DIR/libpthread.so.0"
rm -f "$LIB_DIR/libdl.so.2" "$LIB_DIR/librt.so.1" "$LIB_DIR/libstdc++.so.6" "$LIB_DIR/libz.so.1" "$LIB_DIR/libgcc_s.so.1"

# fix ./uSmartView_VDI_Client: error while loading shared libraries: libbz2.so.1.0: cannot open shared object file: No such file or directory
cp /usr/lib/x86_64-linux-gnu/libbz2.so.1 "$CCSDK_ZTE_DIR/lib/libbz2.so.1.0"

# try fix log file access error
rm -rf "$LOG_DIR"
rm -rf "$CCSDK_ZTE_DIR/log"
ln -sf /var/cache/vdi_log "$LOG_DIR"
ln -sf /var/cache/vdi_log "$CCSDK_ZTE_DIR/log"
rm -rf "$APP_ROOT/teml"
ln -sf /var/cache/teml "$APP_ROOT/teml"

rm -rf cloudpc.deb usr opt data.* control.tar.gz debian-binary
