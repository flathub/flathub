#!/usr/bin/env bash

clear
flatpak-builder --repo=testing-repo --force-clean build-dir org.gnome.FontManager.yaml
flatpak --user remote-add --if-not-exists --no-gpg-verify fv-testing-repo testing-repo
flatpak --user install fv-testing-repo org.gnome.FontManager -y
flatpak --user install fv-testing-repo org.gnome.FontManager.Debug -y
flatpak update -y

