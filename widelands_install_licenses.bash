#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'install licenses';
install -d "/app/share/licenses/${app_name}";
install -p -m 0644 "COPYING" "/app/share/licenses/${app_name}/";
echo;

