#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';
echo 'edit appdata';
xmlstarlet_home='/app/opt/xmlstarlet';
xmlstarlet_bin="${xmlstarlet_home}/bin";
if [[ -n "${PATH}" ]]; then
  PATH="${PATH}:${xmlstarlet_bin}";
else
  PATH="${xmlstarlet_bin}";
fi;
export PATH;
xmlstarlet ed --inplace -d '/component/releases/release[position()>1]' "${app_name}.appdata.xml";
echo;

