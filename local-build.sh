#!/usr/bin/env bash

rm -f com.adobe.Flash-Player-Projector.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build com.adobe.Flash-Player-Projector.json --repo=_repo
