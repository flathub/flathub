#!/usr/bin/env bash

set -euo pipefail

# unpack debian package
bsdtar -Oxf "Breitbandmessung-linux.deb" 'data.tar.*' |
    bsdtar --include 'opt/*' -xf -

# clean up
rm Breitbandmessung-linux.deb
