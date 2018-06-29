#!/usr/bin/env bash

workspace="$( dirname "$( readlink -f "${0}" )" )"

cd "${workspace}"

[[ -x "./flatpak_setup.bash" ]] && source "./flatpak_setup.bash" || { echo "Failed to initialize script." >&2; exit 1; }

status_strip="0"
status_sed="0"

while IFS= read -r -d '' filename; do
  filename_input="$( basename "${filename}" )"
  filename_output="${filename_input%.in}"
  strip-json-comments "${filename_input}"  > "${filename_output}"
  [[ "${?}" -eq "0" ]] || status_strip="1"
  sed -i '/^[[:blank:]]*$/d' "${filename_output}"
  [[ "${?}" -eq "0" ]] || status_sed="2"
done < <( find . -mindepth 1 -maxdepth 1 -xtype f -name '*.json.in' -print0 | sort -V -z )

exit "$(( ${status_strip} + ${status_sed} ))"

