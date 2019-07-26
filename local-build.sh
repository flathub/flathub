#!/usr/bin/env bash

rm -f com.carpeludum.KegaFusion.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --arch i386 --ccache --force-clean _build com.carpeludum.KegaFusion.json --repo=_repo
flatpak build-bundle --arch i386 _repo com.carpeludum.KegaFusion.flatpak com.carpeludum.KegaFusion master
