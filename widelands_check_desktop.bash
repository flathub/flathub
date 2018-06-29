#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'check desktop';
desktop-file-validate "/app/share/applications/${app_name}.desktop";
echo "status: ${?}";
echo;

