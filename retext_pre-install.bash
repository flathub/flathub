#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './retext_setup.bash';

echo 'create: manuals';
mkdir -p 'man/de';
perllib_path="$( find po4a-*/lib -mindepth 0 -maxdepth 0 -xtype d | sort -V | head -n 1 )";
echo "${perllib_path}";
PERLLIB="${perllib_path}" ./po4a-0.52/po4a-updatepo -v -M utf-8 -f man -m "${app_name}.1" -p "${app_name}-man-de.po";
PERLLIB="${perllib_path}" ./po4a-0.52/po4a-translate -M utf-8 -f man --option groff_code=verbatim -m "${app_name}.1" -p "${app_name}-man-de.po" -l "man/de/${app_name}.1";
echo;

echo 'create icons';
icon="icons/${app_name}.svg";
for s in {16,22,24,32,48,64,72,96,128,192,256,512,scalable}; do
  if [[ "${s}" == "scalable" ]]; then
    size="${s}";
  else
    size="${s}x${s}";
  fi;
  echo "- ${size}";
  mkdir -p "icons_flatpak/${size}/";
  if [[ "${s}" == "scalable" ]]; then
    cp "${icon}" "icons_flatpak/${size}/${app_name}.svg";
    continue;
  fi
  rsvg-convert "${icon}" -w "${s}" -h "${s}" -a -f png -o "icons_flatpak/${size}/${app_name}.png";
done;
echo;

echo 'create desktop';
cp "data/${app_id}.desktop" "${app_name}.desktop";
echo;

echo 'create appdata';
cp "data/${app_id}.appdata.xml" "${app_name}.appdata.xml";
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
filename="${app_name}.appdata.xml";
app_version="$( grep -Ei "^[[:blank:]]*VERSION[[:blank:]]*=[[:blank:]]*'([^']*)'*[[:blank:]]*$" 'setup.py' 2>/dev/null | tail -n 1 2>/dev/null | sed -re "s/^[[:blank:]]*VERSION[[:blank:]]*=[[:blank:]]*'([^']*)'*[[:blank:]]*$/\1/" 2>/dev/null )";
app_date="$( date --reference 'changelog.md' -u '+%Y-%m-%d' 2>/dev/null )";
if [[ -n "${app_version}" && -n "${app_date}" ]]; then
    xmlstarlet sel -t -v '/component/releases/release/@version' "${filename}";
    if [[ "${?}" -ne "0" ]]; then
        xmlstarlet ed --inplace -d '/component/releases' -s '/component' -t elem -n 'releases' -s '/component/releases' -t elem -n 'release' -s '/component/releases/release' -t attr -n 'date' -v "${app_date}" -s '/component/releases/release' -t attr -n 'version' -v "${app_version}" "${filename}";
        xmlstarlet sel -t -c '/component/releases/release' "${filename}" | sed -re 's/$/\n/' || :;
    fi
fi;
app_id="$( xmlstarlet sel -t -v '/component/id' "${filename}" 2>/dev/null | head -n 1 2>/dev/null )";
if [[ -n "${app_id}" ]]; then
    xmlstarlet sel -t -v '/component/launchable' "${filename}";
    if [[ "${?}" -ne "0" ]]; then
        xmlstarlet ed --inplace -d '/component/launchable' -s '/component' -t elem -n 'launchable' -v "${app_id}" -s '/component/launchable' -t attr -n 'type' -v 'desktop-id' "${filename}";
        xmlstarlet sel -t -c '/component/launchable' "${filename}" | sed -re 's/$/\n/' || :;
    fi
fi;
echo;

