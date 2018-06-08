#!/bin/bash
set -e

flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.freedesktop.Platform//1.6 org.freedesktop.Sdk//1.6
flatpak install flathub org.electronjs.Electron2.BaseApp
