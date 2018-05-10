#!/usr/bin/env bash

workspace="$( dirname "$( readlink -f "${0}" )" )"

cd "${workspace}"

dirname_build="build"
filename_json="com.notepadqq.Notepadqq.json"
command_exec="notepadqq"

flatpak-builder --run "${dirname_build}" "${filename_json}" "${command_exec}"
