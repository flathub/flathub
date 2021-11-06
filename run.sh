#!/usr/bin/env bash

clear
flatpak-builder --repo=testing-repo --force-clean build-dir com.faforever.Client.yaml
flatpak --user remote-add --if-not-exists --no-gpg-verify faf-testing-repo testing-repo
flatpak --user install faf-testing-repo com.faforever.Client -y
flatpak --user install faf-testing-repo com.faforever.Client.Debug -y
flatpak update -y

