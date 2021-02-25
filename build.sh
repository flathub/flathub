#!/bin/bash

set -e
set -x

YML=com.valvesoftware.SteamLink.yml
while :; do
    case $1 in
        --yml) YML=$2
        ;;
        *) break
    esac
    shift
done

mkdir -p _source
flatpak-builder \
    --force-clean \
    --repo=_build/repo \
    --extra-sources=_source \
    _build \
    ${YML}

flatpak \
    build-bundle \
    _build/repo \
    _build/steamlink.flatpak \
    com.valvesoftware.SteamLink

echo "Install with: flatpak install --bundle /path/to/steamlink.flatpak"
