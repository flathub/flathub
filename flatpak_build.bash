#!/usr/bin/env bash

workspace="$( dirname "$( readlink -f "${0}" )" )"

cd "${workspace}"

[[ -x "./flatpak_setup.bash" ]] && source "./flatpak_setup.bash" || { echo "Failed to initialize script." >&2; exit 1; }

dirname_build="build"

#rm -rf "${dirname_build}"
mkdir -p "${dirname_build}"
flatpak-builder "${dirname_build}" "${flatpak_manifest}" --force-clean --install-deps-from="flathub"

