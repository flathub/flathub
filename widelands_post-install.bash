#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';

echo 'install icons';
find 'icons' -mindepth 2 -maxdepth 2 -type f -name "${app_name}.png" | sort -V | xargs -I{} dirname '{}' | xargs -I{} basename '{}' | while read -r size; do
  install -p -D -m 0644 "icons/${size}/${app_name}.png" "/app/share/icons/hicolor/${size}/apps/${app_name}.png";
done;
echo;

echo 'install pixmaps';
install -d "/app/share/pixmaps";
ln -s "../${app_name}/images/logos/wl-logo-64.png" "/app/share/pixmaps/${app_name}.png";
echo;

echo 'install desktop';
install -p -D -m 0644 "${app_name}.desktop" "/app/share/applications/${app_name}.desktop";
echo;

echo 'install appdata';
install -p -D -m 0644 "${app_name}.appdata.xml" "/app/share/metainfo/${app_name}.appdata.xml";
echo;

echo 'install docs';
install -d "/app/share/doc/${app_name}";
install -p -m 0644 "ChangeLog" "CREDITS" "WL_RELEASE" "/app/share/doc/${app_name}/";
echo;

echo 'install licenses';
install -d "/app/share/licenses/${app_name}";
install -p -m 0644 "COPYING" "/app/share/licenses/${app_name}/";
echo;

echo 'check desktop';
desktop-file-validate "/app/share/applications/${app_name}.desktop";
echo "status: ${?}";
echo;

echo 'check appdata';
appstream-util validate-relax --nonet "/app/share/metainfo/${app_name}.appdata.xml";
echo "status: ${?}";
echo;

