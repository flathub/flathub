#!/usr/bin/env bash

rm -f com.github.shonumi.gbe-plus.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build com.github.shonumi.gbe-plus.json --repo=_repo
flatpak build-bundle _repo com.github.shonumi.gbe-plus.flatpak com.github.shonumi.gbe-plus master

