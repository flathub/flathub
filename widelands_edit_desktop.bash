#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'edit desktop';
desktop-file-edit --set-key='Icon' --set-value="${app_name}" "${app_name}.desktop";
echo;

