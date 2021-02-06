#!/usr/bin/env bash

rm -f tech.feliciano.pocket-casts.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build tech.feliciano.pocket-casts.yml --repo=_repo
flatpak build-bundle _repo tech.feliciano.pocket-casts.flatpak tech.feliciano.pocket-casts master
