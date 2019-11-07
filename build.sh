#!/usr/bin/env bash

set -e

app_id="moe._999eagle.OkunaDesktop"

mkdir -p "/app/opt"
mkdir -p "/app/bin"
mkdir -p "/app/share/applications"
mkdir -p "/app/share/metainfo"

echo "copying files"

# copy main okuna files
mv okuna-desktop /app/opt/

# set up entry point
mv okuna-desktop.sh /app/bin/
rm /app/opt/okuna-desktop/okuna-desktop.sh

# set up desktop entry
mv "/app/opt/okuna-desktop/okuna-desktop.desktop" "/app/share/applications/${app_id}.desktop"
sed -i "s/^Exec=.*$/Exec=okuna-desktop.sh/" "/app/share/applications/${app_id}.desktop"
sed -i "s/^Icon=.*$/Icon=${app_id}/" "/app/share/applications/${app_id}.desktop"

# set up icons
for size in 32 64 256; do
	path="/app/share/icons/hicolor/${size}x${size}/apps"
	mkdir -p "${path}"
	cp "/app/opt/okuna-desktop/assets/okuna-o-logo_transparent_${size}.png" "${path}/${app_id}.png"
done

# set up appdata
mv "${app_id}.appdata.xml" "/app/share/metainfo/${app_id}.appdata.xml"
