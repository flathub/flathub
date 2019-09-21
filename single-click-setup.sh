#!/bin/bash

flatpak-builder build-dir/ com.github.Flacon.yaml --force-clean
flatpak-builder --repo=repo --force-clean build-dir com.github.Flacon.yaml
flatpak --user remote-add --no-gpg-verify --if-not-exists tutorial-repo repo
flatpak install com.github.Flacon -y
flatpak update -y

