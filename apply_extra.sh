#!/usr/bin/env sh

set -o errexit

tar --extract --file=jetbrains-toolbox-app.tar.gz --gunzip --strip-components=1
unappimage jetbrains-toolbox
install --directory --mode=0755 toolbox-app/
mv squashfs-root/* toolbox-app/
rm --force --recursive squashfs-root/ jetbrains-toolbox jetbrains-toolbox-app.tar.gz
