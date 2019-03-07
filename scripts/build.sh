#!/bin/bash

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd "${ROOT_DIR}/.." > /dev/null
flatpak-builder \
    --force-clean \
    --ccache \
    --require-changes \
    --repo=repo \
    --arch=$(flatpak --default-arch) \
    --subject="build of br.gov.cti.invesalius, $(date)" \
    --sandbox \
    build \
    br.gov.cti.invesalius.json
popd > /dev/null
