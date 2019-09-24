#!/bin/bash

flatpak-builder build-dir/ org.yamagi.YamagiQ2.yaml --force-clean
flatpak-builder --repo=repo --force-clean build-dir org.yamagi.YamagiQ2.yaml
flatpak --user remote-add --no-gpg-verify --if-not-exists tutorial-repo repo
flatpak install org.yamagi.YamagiQ2 -y
flatpak update -y
