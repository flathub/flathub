#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'install docs';
install -d "/app/share/doc/${app_name}";
install -p -m 0644 "ChangeLog" "CREDITS" "WL_RELEASE" "/app/share/doc/${app_name}/";
echo;

