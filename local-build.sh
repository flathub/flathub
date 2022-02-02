#!/usr/bin/env bash

BRANCH=test

rm -f org.gnome.Totem.Devel.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean --default-branch=$BRANCH _build org.gnome.Totem.Devel.json --repo=_repo
