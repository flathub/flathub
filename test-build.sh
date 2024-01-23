#!/usr/bin/env bash

clear
flatpak-builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/repo/screenshots --repo=repo builddir  org.serioussam.SeriousSamClassic-VK.yaml
