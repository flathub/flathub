#! /bin/sh

rm -rf build export .flatpak-builder &&
flatpak-builder build org.citron_emu.citron.json &&
flatpak build-export export build &&
exec flatpak build-bundle export citron.flatpak org.citron_emu.citron --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
