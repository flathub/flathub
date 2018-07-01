#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './retext_setup.bash';

echo 'install manuals';
install -d "/app/share/man/man1";
install -p -m 0644 "${app_name}.1" "/app/share/man/man1/";
install -d "/app/share/man/de/man1";
install -p -m 0644 "man/de/${app_name}.1" "/app/share/man/de/man1/";
echo;

echo 'install icons';
find 'icons_flatpak' -mindepth 2 -maxdepth 2 -type f -name "${app_name}.png" | sort -V | xargs -I{} dirname '{}' | xargs -I{} basename '{}' | while read -r size; do
  install -p -D -m 0644 "icons_flatpak/${size}/${app_name}.png" "/app/share/icons/hicolor/${size}/apps/${app_name}.png";
done;
echo;

echo 'install pixmaps';
install -d "/app/share/pixmaps";
install -p -D -m 0644 "icons/${app_name}.png" "/app/share/pixmaps/${app_name}.png";
echo;

echo 'install desktop';
[[ ! -f "/app/share/applications/${app_id}.desktop" ]] || rm -f "/app/share/applications/${app_id}.desktop";
install -p -D -m 0644 "${app_name}.desktop" "/app/share/applications/${app_name}.desktop";
echo;

echo 'install appdata';
[[ -d "/app/share/metainfo" ]] || mv "/app/share/appdata" "/app/share/metainfo";
[[ ! -f "/app/share/metainfo/${app_id}.appdata.xml" ]] || rm -f "/app/share/metainfo/${app_id}.appdata.xml";
install -p -D -m 0644 "${app_name}.appdata.xml" "/app/share/metainfo/${app_name}.appdata.xml";
echo;

echo 'install docs';
install -d "/app/share/doc/${app_name}";
install -p -m 0644 "changelog.md" "configuration.md" "README.md" "/app/share/doc/${app_name}/";
echo;

echo 'install licenses';
install -d "/app/share/licenses/${app_name}";
install -p -m 0644 "LICENSE_GPL" "/app/share/licenses/${app_name}/";
echo;

echo 'check desktop';
desktop-file-validate "/app/share/applications/${app_name}.desktop";
echo "status: ${?}";
echo;

echo 'check appdata';
appstream-util validate-relax --nonet "/app/share/metainfo/${app_name}.appdata.xml";
echo "status: ${?}";
echo;

