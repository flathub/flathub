#!/usr/bin/env bash

rm -f org.freecadweb.FreeCAD.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder --ccache --force-clean _build org.freecadweb.FreeCAD.yaml --repo=_repo
flatpak build-bundle _repo org.freecadweb.FreeCAD.flatpak org.freecadweb.FreeCAD master

