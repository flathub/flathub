#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'install desktop';
install -p -D -m 0644 "${app_name}.desktop" "/app/share/applications/${app_name}.desktop";
echo;

