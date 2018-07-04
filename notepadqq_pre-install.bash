#!/usr/bin/env bash

cd "$( dirname "$( readlink -f "${0}" )" )";
source './notepadqq_setup.bash';

echo 'edit desktop';
filename="support_files/shortcuts/${app_name}.desktop";
desktop-file-edit --set-key='Icon' --set-value="${app_name}" "${filename}";
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
filename="support_files/${app_name}.appdata.xml";
app_version="$( grep -Ei "^[[:blank:]]*version[[:blank:]]*:[[:blank:]]*'([^']*)'*[[:blank:]]*$" 'snap/snapcraft.yaml' 2>/dev/null | tail -n 1 2>/dev/null | sed -re "s/^[[:blank:]]*version[[:blank:]]*:[[:blank:]]*'([^']*)'*[[:blank:]]*$/\1/" 2>/dev/null || : )";
app_date="$( date --reference 'src' -u '+%Y-%m-%d' 2>/dev/null || : )";
echo 'fixing xml declaration';
sed -r -i '1 s/^[[:blank:]]+//' "${filename}" || :;
echo 'removing invalid tag';
xmlstarlet ed --inplace -d '/component/icon' "${filename}" || :;
if [[ -n "${app_version}" && -n "${app_date}" ]]; then
  need_fix='0';
  xmlstarlet sel -t -v '/component/releases/release/@version' "${filename}" || need_fix='1';
  if [[ "${need_fix}" -ne "0" ]]; then
    echo 'updating release info';
    xmlstarlet ed --inplace -d '/component/releases' -s '/component' -t elem -n 'releases' -s '/component/releases' -t elem -n 'release' -s '/component/releases/release' -t attr -n 'date' -v "${app_date}" -s '/component/releases/release' -t attr -n 'version' -v "${app_version}" "${filename}";
    xmlstarlet sel -t -c '/component/releases/release' "${filename}" | sed -re 's/$/\n/' || :;
  fi;
fi;
if [[ -n "${app_id}" ]]; then
  xmlstarlet ed --inplace -d '/component/id' -s '/component' -t elem -n 'id' -v "${app_id}" "${filename}";
  need_fix='0';
  xmlstarlet sel -t -v '/component/launchable' "${filename}" || need_fix='2';
  if [[ "${need_fix}" -ne "0" ]]; then
    echo 'adding additional tags';
    xmlstarlet ed --inplace -d '/component/launchable' -s '/component' -t elem -n 'launchable' -v "${app_id}" -s '/component/launchable' -t attr -n 'type' -v 'desktop-id' "${filename}";
    xmlstarlet sel -t -c '/component/launchable' "${filename}" | sed -re 's/$/\n/' || :;
  fi;
fi;
echo;

