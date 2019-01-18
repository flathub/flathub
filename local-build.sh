#!/usr/bin/env bash

rm -f org.gnome.Totem.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build org.gnome.Totem.json --repo=_repo
