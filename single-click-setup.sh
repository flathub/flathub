#!/bin/bash

flatpak-builder build-dir/ io.github.ezQuake.yaml --force-clean
flatpak-builder --repo=repo --force-clean build-dir io.github.ezQuake.yaml
flatpak --user remote-add --no-gpg-verify --if-not-exists tutorial-repo repo
flatpak install io.github.ezQuake -y
flatpak update -y
