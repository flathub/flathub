#!/usr/bin/bash
pyenv local 3.11

flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo builddir io.github.qcanvas.QCanvasApp.yaml

if [ "$1" != "no-run" ]; then
    flatpak run io.github.qcanvas.QCanvasApp
fi

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.qcanvas.QCanvasApp.yaml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
