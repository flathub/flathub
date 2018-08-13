#!/usr/bin/env bash

rm -f net.sourceforge.Teo.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build net.sourceforge.Teo.json --repo=_repo
flatpak build-bundle _repo net.sourceforge.Teo.flatpak net.sourceforge.Teo master

