#!/usr/bin/env sh

set -e

bsdtar -Oxf sketcher.deb 'data.tar.*' |
  bsdtar -xf - \
    --strip-components=2 \
    --exclude='./etc/' \
    --exclude='./usr/bin/' \
    --exclude='./usr/share/doc/'

mv WireframeSketcher ${FLATPAK_DEST}/

find share -type f -exec install -Dm644 "{}" "${FLATPAK_DEST}/{}" \;

install -Dm644 flatpak-dev-shim.jar ${FLATPAK_DEST}/flatpak-dev-shim.jar
install -Dm644 flatpak-dev-shim.so ${FLATPAK_DEST}/lib/libflatpakdevshim.so

mkdir ${FLATPAK_DEST}/bin
ln -s ${FLATPAK_DEST}/WireframeSketcher/WireframeSketcher ${FLATPAK_DEST}/bin/WireframeSketcher

sed -i -e 's/<\/component>/\t<launchable type="desktop-id">wireframesketcherstudio.desktop<\/launchable>\n\t<content_rating type="oars-1.1"\/>\n<\/component>/g' ${FLATPAK_DEST}/share/appdata/wireframesketcherstudio.appdata.xml

desktop-file-edit --set-key=Exec --set-value=WireframeSketcher --set-key=TryExec --set-value=WireframeSketcher ${FLATPAK_DEST}/share/applications/wireframesketcherstudio.desktop 

INI_FILE=${FLATPAK_DEST}/WireframeSketcher/WireframeSketcher.ini

sed -i -e '/osgi.configuration.area/d' $INI_FILE
echo "-Dosgi.configuration.area=@user.home/.var/app/${FLATPAK_ID}/WireframeSketcher/configuration" >> $INI_FILE
echo "--patch-module=java.base=/app/flatpak-dev-shim.jar" >> $INI_FILE
echo "-Dsun.boot.library.path=/app/lib" >> $INI_FILE
