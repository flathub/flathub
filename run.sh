#!/usr/bin/env bash

sudo rm -R build-dir && \
sudo rm -R .flatpak-builder && \
flatpak uninstall io.github.nate_xyz.Conjure && \
flatpak-builder build-dir io.github.nate_xyz.Conjure.json && \
flatpak-builder --user --install --force-clean build-dir io.github.nate_xyz.Conjure.json && \
flatpak run io.github.nate_xyz.Conjure