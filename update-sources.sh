#!/usr/bin/bash
#
# This script uses the version set up in the Headlamp flatpak manifest to
# generate the dependencies files (*-source-generated*.json) for the build.
# It can also be used for printing the version currently set in the manifest.
#
# Copyright Kinvolk GmbH 2021
#
# Author: Joaquim Rocha <joaquim@kinvolk.io>
#

set -eu

INFRA_DIR=$(realpath `dirname $0`)
FLATPAK_BUILDER_TOOLS=${FLATPAK_BUILDER_TOOLS:-"$INFRA_DIR"/flatpak-builder-tools}
GO_SOURCES_GENERATOR="$FLATPAK_BUILDER_TOOLS"/go-get/flatpak-go-vendor-generator.py
NODE_SOURCES_GENERATOR="$FLATPAK_BUILDER_TOOLS"/node/flatpak-node-generator.py

YAML="io.kinvolk.Headlamp.yaml"
REPO=$(cat "$YAML" | grep -A2 -B2 'kinvolk/headlamp.git' | grep '   url:' | sed -e s/\s*url:// | xargs)
VERSION=$(cat "$YAML" | grep -A2 -B2 'kinvolk/headlamp.git' | grep '   branch:' | cut -d ':' -f 2 | xargs)

function usage(){
  echo "$0 [VERSION,-v]    : Update the generated sources for Headlamp"
  echo
  echo "  -v   print current tag/version"
}

function make_go_sources(){
    pushd backend
    go mod vendor
    $GO_SOURCES_GENERATOR ./vendor/modules.txt > "$INFRA_DIR/go-generated-sources.json"
    popd
}

function make_node_sources(){
    $NODE_SOURCES_GENERATOR --xdg-layout -o "$INFRA_DIR/node-generated-sources-frontend.json" npm ./frontend/package-lock.json
    $NODE_SOURCES_GENERATOR --xdg-layout -o "$INFRA_DIR/node-generated-sources-app.json" npm ./app/package-lock.json
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
  usage
  exit 1
fi

if [ "${1:-}" = "-v" ]; then
  echo "$VERSION"
  exit 0
fi

if [ ! $(command -v "$GO_SOURCES_GENERATOR") ]; then
  echo "Cannot find $(basename $GO_SOURCES_GENERATOR). Set the FLATPAK_BUILDER_TOOLS env var to point to a flatpak-builder-tools checkout."
  exit 1
fi

if [ ! $(command -v "$NODE_SOURCES_GENERATOR") ]; then
  echo "Cannot find $(basename $NODE_SOURCES_GENERATOR). Set the FLATPAK_BUILDER_TOOLS env var to point to a flatpak-builder-tools checkout."
  exit 1
fi

PATH="$INFRA_DIR"/flatpak-builder-tools:$PATH

TMP_DIR=$(mktemp -d -u "headlamp-flatpak.XXXXXXXXX")

git clone --depth=1 --branch="$VERSION" "$REPO" "$TMP_DIR"
pushd "$TMP_DIR"

make_go_sources
make_node_sources

popd

if [ -d "$TMP_DIR" ]; then
  rm -rf "$TMP_DIR"
fi
