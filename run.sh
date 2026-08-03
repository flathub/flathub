#!/usr/bin/env bash

clear
flatpak-builder --repo=testing-repo --force-clean build-dir net.qcde.QCDE.yaml
flatpak --user remote-add --if-not-exists --no-gpg-verify qcde-testing-repo testing-repo
flatpak --user install qcde-testing-repo net.qcde.QCDE -y
flatpak update -y

