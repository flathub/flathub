#!/bin/bash
set -e

mkdir -p wps-office export/share

tar -xJf wps.tar.xz --strip-components=1 -C wps-office

cp -ax wps-office/resource/{icons,applications,mime} export/share/
rm export/share/applications/appurl.desktop

for f in export/share/{icons/hicolor/*/*,applications,mime/packages}/wps-office-*.*; do
    mv "$f" "${f/wps-office-/com.wps.Office.}";
done
sed -i 's|/opt/kingsoft/wps-office|/app/extra/wps-office|g' -i wps-office/{wps,wpp,et}
sed -i 's|Exec=/usr/bin/|Exec=|g' -i export/share/applications/com.wps.Office.*.desktop
sed -i 's/Icon=wps-office-/Icon=com.wps.Office./' export/share/applications/com.wps.Office.*.desktop
sed -i 's/generic-icon name="wps-office-/icon name="com.wps.Office./g' export/share/mime/packages/com.wps.Office.*.xml

for l in /app/share/wps/office6/mui/*; do
    d=$(basename $l)
    test -d wps-office/office6/mui/$d || ln -sr /app/share/wps/office6/mui/$d wps-office/office6/mui/$d
done

rm wps.tar.xz
