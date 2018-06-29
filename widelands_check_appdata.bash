#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'check appdata';
appstream-util validate-relax --nonet "/app/share/metainfo/${app_name}.appdata.xml";
echo "status: ${?}";
echo;

