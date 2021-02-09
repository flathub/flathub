#!/usr/bin/env bash

rm -f org.ghidra_sre.Ghidra.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build org.ghidra_sre.Ghidra.json --repo=_repo
