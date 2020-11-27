#!/usr/bin/env bash

BRANCH=test

rm -f com.poweriso.PowerISO.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean --default-branch=$BRANCH _build com.poweriso.PowerISO.json --repo=_repo
flatpak build-bundle _repo com.poweriso.PowerISO.flatpak com.poweriso.PowerISO $BRANCH

