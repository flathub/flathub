#!/bin/sh
# Installs Darling's official Ubuntu packages into /app.
#
# The Linux programs have their install paths built in: /usr/libexec/darling
# (the macOS root, where mldr and vchroot live too) and /usr/bin/darlingserver.
# /app/... has the same length, so the strings are replaced in place. Only
# the leading one: /usr/libexec/darling/usr/libexec/darling/mldr is mldr inside
# the macOS root.
set -eu
zip=$1
work=$(mktemp -d)
bsdtar -xf "$zip" -C "$work"
for deb in "$work"/debs_*/*.deb; do
    case $(basename "$deb") in
        # Script interpreters; Roblox needs none of them.
        darling-perl_*|darling-python2_*|darling-ruby_*|darling-pyobjc_*|darling-cli-python2-*)
            continue ;;
    esac
    mkdir -p "$work/deb"
    bsdtar -xf "$deb" -C "$work/deb" 'data.tar.*'
    bsdtar -xf "$work"/deb/data.tar.* -C "$work"
    rm -rf "$work/deb"
done
mkdir -p /app/bin /app/libexec
cp -a "$work"/usr/bin/darling "$work"/usr/bin/darlingserver /app/bin/
cp -a "$work"/usr/libexec/darling /app/libexec/
for program in /app/bin/darling /app/bin/darlingserver; do
    sed -i -e 's|/usr/libexec/darling|/app/libexec/darling|g' \
           -e 's|/app/libexec/darling/app/libexec/darling|/app/libexec/darling/usr/libexec/darling|g' \
           -e 's|/usr/bin/darlingserver|/app/bin/darlingserver|g' "$program"
done
# The Flatpak cannot use the setuid bit; darling-noroot.so stands in for root.
chmod 0755 /app/bin/darling
rm -rf "$work"
