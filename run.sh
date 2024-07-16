#!/usr/bin/env bash

clear
flatpak-builder --repo=testing-repo --force-clean build-dir com.github.FontManager.FontManager.yaml
flatpak --user remote-add --if-not-exists --no-gpg-verify fm-testing-repo testing-repo
flatpak --user install fm-testing-repo com.github.FontManager.FontManager -y
flatpak --user install fm-testing-repo com.github.FontManager.FontManager.Debug -y
flatpak update -y
