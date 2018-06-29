#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'install';
./widelands_install_icons.bash;
./widelands_install_pixmaps.bash;
./widelands_install_desktop.bash;
./widelands_install_appdata.bash;
./widelands_install_docs.bash;
./widelands_install_licenses.bash;
echo;

