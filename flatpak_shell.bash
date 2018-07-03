#!/usr/bin/env bash

workspace="$( dirname "$( readlink -f "${0}" )" )"

cd "${workspace}"

[[ -x "./flatpak_setup.bash" ]] && source "./flatpak_setup.bash" || { echo "Failed to initialize script." >&2; exit 1; }

dirname_build="build"
filename_json="${flatpak_id}.json"
command_exec="sh"

flatpak-builder --run "${dirname_build}" "${filename_json}" "${command_exec}"

