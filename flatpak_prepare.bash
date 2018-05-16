#!/usr/bin/env bash

workspace="$( dirname "$( readlink -f "${0}" )" )"

cd "${workspace}"

filename_input="widelands.json.in"
filename_output="org.widelands.widelands.json"

strip-json-comments "${filename_input}"  > "${filename_output}"
status_strip="${?}"
sed -i '/^[[:blank:]]*$/d' "${filename_output}"
status_sed="${?}"
[[ "${status_sed}" -eq "0" ]] || exit "${status_sed}"
exit "${status_strip}"
