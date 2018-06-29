#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'install pixmaps';
install -d "/app/share/pixmaps";
ln -s "../${app_name}/images/logos/wl-logo-64.png" "/app/share/pixmaps/${app_name}.png";
echo;

