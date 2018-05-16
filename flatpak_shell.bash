#!/usr/bin/env bash

workspace="$( dirname "$( readlink -f "${0}" )" )"

cd "${workspace}"

dirname_build="build"
filename_json="org.widelands.widelands.json"
command_exec="sh"

flatpak-builder --run "${dirname_build}" "${filename_json}" "${command_exec}"
