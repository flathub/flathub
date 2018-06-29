#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'install appdata';
install -p -D -m 0644 "${app_name}.appdata.xml" "/app/share/metainfo/${app_name}.appdata.xml";
echo;

