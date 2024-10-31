#!/bin/sh
flatpak remote-delete repo
rm -r repo
flatpak-builder --user --sandbox --ccache --force-clean --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo build-dir io.github.eclab.edisyn.yml
flatpak remote-add --user --if-not-exists --no-gpg-verify repo repo
flatpak install io.github.eclab.edisyn