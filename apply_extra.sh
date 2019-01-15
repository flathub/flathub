#!/bin/bash -x
tar -xJf wps-office.tar.xz --strip-components=1

mkdir -p export/share
mv resource/* export/share/

for f in export/share/{icons/hicolor/*/*,applications,mime/packages}/wps-office-*.*; do
    mv "$f" "${f/wps-office-/com.wps.Office-}";
done
sed -i 's|/opt/kingsoft/wps-office|/app/extra|g' -i {wps,wpp,et}
sed -i 's|Exec=/usr/bin/|Exec=|g' -i export/share/applications/com.wps.Office-*.desktop
sed -i 's/Icon=wps-office/Icon=com.wps.Office/' export/share/applications/com.wps.Office-*.desktop
sed -i 's/generic-icon name="wps-office-/icon name="com.wps.Office-/g' export/share/mime/packages/com.wps.Office-*.xml

for l in /app/share/wps/office6/mui/*; do
    d=$(basename $l)
    test -d office6/mui/$d || ln -sr /app/share/wps/office6/mui/$d office6/mui/$d
done

rm wps-office.tar.xz
