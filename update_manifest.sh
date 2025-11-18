#!/bin/bash

VERSION=1.29
GIT_TAG=v1.29.0.2
BASE_GIT_URL=https://github.com/LordOfDragons
BASE_URL_ARTIFACTS=https://dragondreams.s3.eu-central-1.amazonaws.com/dragengine/extern
MANIFEST=ch.dragondreams.delauncher.yml

sed $MANIFEST.in -e "s/%VERSION%/$VERSION/" -e "s/%TAG%/$GIT_TAG/" >$MANIFEST

function writeExternSource() {
	BASE_DIR=$1
	FILENAME=$2
	
	URL=$BASE_URL_ARTIFACTS/$BASE_DIR/$FILENAME
	echo '' >>$MANIFEST
	echo '      - type: file' >>$MANIFEST
	echo "        url: $URL" >>$MANIFEST
	
	SHA256SUM=`curl -L -s $URL.sha256sum`
	echo "        sha256: $SHA256SUM" >>$MANIFEST
	echo "        dest: extern/$BASE_DIR" >>$MANIFEST
	echo "        dest-filename: $FILENAME" >>$MANIFEST
}

function writeExternSourceGit() {
	BASE_DIR=$1
	TAG=$2
	FILENAME=$3
	
	URL=$BASE_GIT_URL/$BASE_DIR/releases/download/$TAG/$FILENAME
	echo '' >>$MANIFEST
	echo '      - type: file' >>$MANIFEST
	echo "        url: $URL" >>$MANIFEST
	
	SHA256SUM=`curl -L -s $URL | sha256sum | head -c 64`
	echo "        sha256: $SHA256SUM" >>$MANIFEST
	echo "        dest: extern/$BASE_DIR" >>$MANIFEST
	echo "        dest-filename: $FILENAME" >>$MANIFEST
}

writeExternSource eossdk eossdk-1.17.0.zip
writeExternSource eossdk eossdk_bin_linux-1.17.0.tar.xz
writeExternSource fox fox-1.7.67.tar.bz2
writeExternSource liburing liburing-liburing-2.6.tar.bz2
writeExternSource modio modio-sdk-2025_10.tar.xz
writeExternSource openxr OpenXR-SDK-release-1.1.46.tar.xz
writeExternSource steamsdk steamsdk160.tar.xz
writeExternSource libapng libpng-1.6.29.tar.bz2
writeExternSource jsoncpp jsoncpp-1.9.6.tar.xz
writeExternSourceGit denetwork v1.2 denetworkcpp-unix-x64-1.2.tar.bz2
writeExternSourceGit deremotelauncher v1.1 deremotelauncher-unix-x64-1.1.tar.bz2
writeExternSource libwebm libwebm-libwebm-1.0.0.32.tar.xz
writeExternSource libevdev libevdev-1.5.6.tar.bz2
writeExternSource dragonscript dragonscript-1.5.tar.xz
writeExternSource soundtouch soundtouch-2.1.1.tar.bz2
writeExternSource openal openal-soft-1.24.2.tar.xz
