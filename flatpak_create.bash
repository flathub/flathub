#!/usr/bin/env bash

workspace="$( dirname "$( readlink -f "${0}" )" )"

cd "${workspace}"

./flatpak_prepare.bash && ./flatpak_build.bash
