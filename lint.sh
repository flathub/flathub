#!/bin/sh
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.eclab.edisyn.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo