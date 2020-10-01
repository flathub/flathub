#!/usr/bin/env bash

rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build fr.handbrake.ghb.Plugin.dvdcss.json --repo=_repo
