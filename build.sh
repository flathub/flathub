#!/bin/bash

rm -rf build-dir repo

flatpak-builder --user --install --mirror-screenshots-url=URL --force-clean \
               --repo=repo build-dir de.stefan_oltmann.mines.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
            manifest de.stefan_oltmann.mines.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
            repo repo

