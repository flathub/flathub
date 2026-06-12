#!/bin/sh
# Runs at install time with the working directory set to /app/extra.
# Unpacks the prebuilt .deb and flattens the app payload into /app/extra so the
# launcher can exec /app/extra/WooCommercePOS.
#
# bsdtar reads the .deb (ar) container and the nested data tarball regardless of
# compression (.xz/.gz/.zst). GNU ar (binutils) is NOT in the Freedesktop
# *Platform* runtime that apply_extra runs against — only the Sdk — so `ar x`
# would fail at install time. This mirrors other extra-data .deb manifests on
# Flathub (e.g. com.google.Chrome).
set -e

bsdtar -Oxf woocommerce-pos.deb 'data.tar.*' | bsdtar -xf -
rm -f woocommerce-pos.deb

# electron-installer-deb lays the app down under /usr/lib/<package-name>/
# (verified against woocommerce-pos_1.9.4_amd64.deb).
mv usr/lib/woocommerce-pos/* .
rm -rf usr
