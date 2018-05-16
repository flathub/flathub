#!/usr/bin/env bash

workspace="$( dirname "$( readlink -f "${0}" )" )"

cd "${workspace}"

dirname_build="build"
filename_json="org.widelands.widelands.json"

rm -rf "${dirname_build}"
mkdir -p "${dirname_build}"
flatpak-builder "${dirname_build}" "${filename_json}" # --install-deps-from=flathub
