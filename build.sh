#!/bin/sh

mkdir -p _source
flatpak-builder \
    --force-clean \
    --repo=_build/repo \
    --extra-sources=_source \
    _build \
    com.valvesoftware.SteamLink.yml

flatpak \
    build-bundle \
    _build/repo \
    _build/steamlink.flatpak \
    com.valvesoftware.SteamLink

echo "Install with: flatpak install --bundle /path/to/steamlink.flatpak"
