#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'check';
./widelands_check_desktop.bash;
./widelands_check_appdata.bash;
echo;

