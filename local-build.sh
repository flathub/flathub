#!/usr/bin/env bash

rm -f com.retrodev.blastem.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build com.retrodev.blastem.json --repo=_repo
flatpak build-bundle _repo com.retrodev.blastem.flatpak com.retrodev.blastem master

