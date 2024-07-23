#!/usr/bin/bash
APP_ID=io.github.qcanvas.QCanvasApp

pyenv local 3.11

# Pypi doesn't always generate package information until someone installs it
utils/nudge-pypi

# Doesn't work without arch-dependent-force!
# pdm run utils/flatpak-pip-generator.py qcanvas --arch-dependent-force
pdm run utils/metainfo-version-updater.py qcanvas $APP_ID.metainfo.xml

git add python3-qcanvas.json $APP_ID.metainfo.xml $APP_ID.yaml
git commit -m "Update manifest"
