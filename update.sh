#!/usr/bin/bash

pyenv local 3.11

pdm run utils/flatpak-pip-generator.py qcanvas --arch-dependent-force
pdm run utils/metainfo-version-updater.py resources/io.github.qcanvas.QCanvasApp.metainfo.xml
