#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './widelands_setup.bash';

echo 'create icons';
icon="${app_name}.svg";
for s in {16,22,24,32,48,64,72,96,128,192,256,512}; do
  size="${s}x${s}";
  echo "- ${size}";
  mkdir -p "icons/${size}/";
  rsvg-convert "${icon}" -w "${s}" -h "${s}" -a -f png -o "icons/${size}/${app_name}.png";
done;
echo;

echo 'create desktop';
cp "debian/${app_id}.desktop" "${app_name}.desktop";
echo;

echo 'create appdata';
cp "debian/${app_name}.appdata.xml" "${app_name}.appdata.xml";
echo;

echo 'edit desktop';
desktop-file-edit --set-key='Icon' --set-value="${app_name}" "${app_name}.desktop";
echo;

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

