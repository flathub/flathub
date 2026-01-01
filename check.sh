#!/bin/bash -xe
#flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo builddir org.dupot.savethesheep.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest org.dupot.savethesheep.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder --exceptions repo repo