#!/usr/bin/env bash

rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

BRANCH=test

flatpak-builder --ccache --force-clean --default-branch=$BRANCH _build io.github.labbots.NiimPrintX.yml --repo=_repo
