#!/usr/bin/env bash

BRANCH=test

rm -f io.github.neil_morrison44.pocket-sync.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean --default-branch=$BRANCH _build io.github.neil_morrison44.pocket-sync.yaml --repo=_repo
flatpak build-bundle _repo io.github.neil_morrison44.pocket-sync.flatpak io.github.neil_morrison44.pocket-sync $BRANCH
