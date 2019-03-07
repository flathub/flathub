#!/bin/bash
set -e

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd "${ROOT_DIR}/.." > /dev/null
scripts/build.sh
flatpak build-bundle repo invesalius.flatpak br.gov.cti.invesalius
popd > /dev/null
