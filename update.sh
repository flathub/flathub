#!/usr/bin/bash
APP_ID=io.github.qcanvas.QCanvasApp

pyenv local 3.11

# Pypi doesn't always generate package information until someone installs it
utils/nudge-pypi

pdm run utils/flatpak-pip-generator.py qcanvas --arch-dependent-force
pdm run utils/metainfo-version-updater.py resources/$APP_ID.metainfo.xml

cd resources || exit
git add $APP_ID.metainfo.xml
git commit -m "Update metainfo"
git push
cd ..

pdm run utils/update-meta-repo.py resources $APP_ID.yaml

git add $APP_ID.yaml python3-qcanvas.json resources
git commit -m "Update manifest"
