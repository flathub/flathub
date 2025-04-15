#!/bin/sh
flatpak install flathub org.flatpak.Builder -y
flatpak install flathub org.freedesktop.Sdk/x86_64/24.08 -y
flatpak run org.flatpak.Builder build --force-clean com.activision.MechWarrior.json
flatpak build-export export build 
flatpak build-bundle export MechWarrior.flatpak com.activision.MechWarrior --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
