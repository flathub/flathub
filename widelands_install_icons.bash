#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'install icons';
find 'icons' -mindepth 2 -maxdepth 2 -type f -name "${app_name}.png" | sort -V | xargs -I{} dirname '{}' | xargs -I{} basename '{}' | while read -r size; do
  install -p -D -m 0644 "icons/${size}/${app_name}.png" "/app/share/icons/hicolor/${size}/apps/${app_name}.png";
done;
echo;

