#!/bin/sh
# Runs at install time inside the sandbox with /app/extra writable.
# Unpack the downloaded .deb (bsdtar reads ar archives; no `ar` in the
# runtime) and keep only what the Flatpak uses.
set -e
bsdtar --to-stdout -xf oxitide.deb 'data.tar.*' | bsdtar -xf -
rm -f oxitide.deb
# Host-only bits (udev rule, polkit action, pkexec helper) don't work
# inside the sandbox — USB Rawlink is native-packages only.
rm -rf usr/lib/udev usr/libexec usr/share/polkit-1 usr/share/applications
# The app looks for an `icons/hicolor` directory next to (or above) its
# executable; give it one at /app/extra/icons.
ln -sfn usr/share/oxitide/icons icons
