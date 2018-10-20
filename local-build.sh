#!/usr/bin/env bash

rm -f ru.sview.sView.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build ru.sview.sView.json --repo=_repo
flatpak build-bundle _repo ru.sview.sView.flatpak ru.sview.sView master
