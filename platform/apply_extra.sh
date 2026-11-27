#!/bin/sh
# Keeps only the application jar from the upstream deb in /app/extra.
set -e

deb=adalet-eimza-tray.deb

bsdtar -Oxf "$deb" 'data.tar*' |
    bsdtar -xf - --strip-components=5 'usr/lib/adalet-eimza-tray/lib/app/eimza-tray-*.jar'
rm "$deb"

ls eimza-tray-*.jar >/dev/null
