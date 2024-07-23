#!/usr/bin/bash

pyenv local 3.11

# pdm run utils/flatpak-pip-generator.py qcanvas==1.0.6a7 --arch-dependent-force
pdm run utils/metainfo-version-updater.py resources/io.github.qcanvas.QCanvasApp.metainfo.xml

cd resources || exit
git commit -m "Update metainfo"
git push
cd ..

pdm run utils/update-meta-repo.py resources io.github.qcanvas.QCanvasApp.yaml

git add io.github.qcanvas.QCanvasApp.yaml python3-qcanvas.json
git commit -m "Update manifest"