#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'edit';
./widelands_edit_desktop.bash;
./widelands_edit_appdata.bash;
echo;

