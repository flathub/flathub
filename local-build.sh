#!/usr/bin/env bash

rm -f org.gnome.OfficeRunner.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build org.gnome.OfficeRunner.json --repo=_repo
flatpak build-bundle _repo org.gnome.OfficeRunner.flatpak org.gnome.OfficeRunner master

