#!/bin/sh
# Runs at install time in /app/extra: unpack the deb downloaded by flatpak.
set -e
bsdtar -Oxf claude-desktop.deb 'data.tar.*' | bsdtar -xf - ./usr/lib/claude-desktop
mv usr/lib/claude-desktop claude-desktop
rm -rf usr claude-desktop.deb
# zypak provides the sandbox, the setuid helper can't work here
rm -f claude-desktop/chrome-sandbox
