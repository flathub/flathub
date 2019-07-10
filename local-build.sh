#!/usr/bin/env bash

rm -f org.ghidra_sre.Desktop.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build org.ghidra_sre.Desktop.json --repo=_repo
flatpak build-bundle _repo org.ghidra_sre.Desktop.flatpak org.ghidra_sre.Desktop stable

