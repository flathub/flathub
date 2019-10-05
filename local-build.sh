#!/usr/bin/env bash

rm -f com.sweethome3d.Sweethome3d.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build com.sweethome3d.Sweethome3d.json --repo=_repo
flatpak build-bundle _repo com.sweethome3d.Sweethome3d.flatpak com.sweethome3d.Sweethome3d stable

