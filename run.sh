#!/usr/bin/env bash

clear
flatpak-builder --repo=testing-repo --force-clean build-dir org.nmap.zenmap.yaml
flatpak --user remote-add --if-not-exists --no-gpg-verify zen-testing-repo testing-repo
flatpak --user install zen-testing-repo org.nmap.zenmap -y
flatpak --user install zen-testing-repo org.nmap.zenmap.Debug -y
flatpak update -y

