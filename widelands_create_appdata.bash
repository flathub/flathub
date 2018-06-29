#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'create appdata';
cp "debian/${app_name}.appdata.xml" "${app_name}.appdata.xml";
echo;

