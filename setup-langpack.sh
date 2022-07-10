#!/bin/bash
set -e
test $PREFIX

echo "Prefix:" $PREFIX
for l in $(ls langpacks/*.xpi)
do
    LOCALE=`basename -s .xpi $l`
    echo "File:" $l "Locale:" $LOCALE
    # Default locale location
    mkdir -p $PREFIX/share/runtime/locale/${LOCALE%%-*}
    echo "Copying $l to $PREFIX/share/runtime/locale/${LOCALE%%-*}/"
    mkdir -p $PREFIX/share/runtime/locale/${LOCALE%%-*}/
    cp $l $PREFIX/share/runtime/locale/${LOCALE%%-*}/ 
    ln -sf "/app/share/runtime/locale/${LOCALE%%-*}/$LOCALE.xpi" "$PREFIX/lib/firefox/distribution/extensions/langpack-${LOCALE}@firefox.mozilla.org.xpi"
done