#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'create desktop';
cp "debian/${app_id}.desktop" "${app_name}.desktop";
echo;

