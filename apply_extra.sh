#!/usr/bin/env sh

set -e

bsdtar -Oxf sketcher.deb 'data.tar.*' |
  bsdtar -xf - \
    --strip-components=2 \
    --exclude='./etc/' \
    --exclude='./usr/'

rm -f sketcher.deb

INI_FILE=WireframeSketcher/WireframeSketcher.ini

sed -i -e '/osgi.configuration.area/d' $INI_FILE
echo "-Dosgi.configuration.area=@user.home/.var/app/com.wireframesketcher.WireframeSketcher/WireframeSketcher/configuration" >> $INI_FILE
echo "--patch-module=java.base=/app/flatpak-dev-shim.jar" >> $INI_FILE
echo "-Dsun.boot.library.path=/app/lib" >> $INI_FILE
