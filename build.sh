#!/bin/bash

flatpak remove com.github.mimillieuh.thinkercad-desktop -y
rm -rf build-dir/ .flatpak-builder/
flatpak-builder --user --install --force-clean build-dir com.github.mimillieuh.thinkercad-desktop.yml

