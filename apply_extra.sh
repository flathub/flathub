#!/bin/sh

set -e

/app/bin/splitelf albion-online-setup albion-online-setup.zip
unzip albion-online-setup.zip 'data/*'
mkdir -p export/share/icons/hicolor/128x128/apps
cp data/AlbionOnline.xpm export/share/icons/hicolor/128x128/apps/com.albiononline.AlbionOnline.xpm
mkdir -p export/share/applications
cp /app/templates/com.albiononline.AlbionOnline.desktop export/share/applications/com.albiononline.AlbionOnline.desktop
