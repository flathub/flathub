#!/usr/bin/env bash

set -e

# Make sure script is started from repo root.
if [ "$(pwd)" != "$(git rev-parse --show-toplevel)" ]; then
    echo -e '\033[31m(ERROR)\033[0m: Script called from wrong dir. Please start from the root of the repository'
    exit 1
fi

# python3 -m flatpak_pip_generator -r requirements.txt --yaml --ignore-pkg pyclipper,PyGObject,pycairo --runtime='org.gnome.Sdk/49' --ignore-errors --output flatpak/python3-requirements

flatpak run org.flatpak.Builder \
    --force-clean --sandbox --user --install --ccache \
    --install-deps-from=flathub \
    --mirror-screenshots-url=https://dl.flathub.org/media/ \
    --repo=repo builddir flatpak/org.rayforge.rayforge.yml
