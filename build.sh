#!/usr/bin/env sh

set -xue

id="com.dbeaver.DBeaverLite"
manifest="${id}.yml"

clear

flatpak install --user -y flathub org.flatpak.Builder
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak run --command=flathub-build org.flatpak.Builder --install "$manifest"
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest "$manifest"
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
flatpak run "$id"
