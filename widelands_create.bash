#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'create';
./widelands_create_icons.bash;
./widelands_create_desktop.bash;
./widelands_create_appdata.bash;
echo;

