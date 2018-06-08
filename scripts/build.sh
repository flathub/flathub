#!/bin/bash

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd "${ROOT_DIR}/.."
flatpak-builder \
    --force-clean \
    --ccache \
    --require-changes \
    --repo=repo \
    --arch=$(flatpak --default-arch) \
    --subject="build of com.github.marktext.marktext, $(date)" \
    --sandbox \
    build \
    com.github.marktext.marktext.json
popd

