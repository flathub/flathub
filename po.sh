#!/bin/sh
APP_ID=io.github.noobping.listenmoe
OUTDIR="${1:-data/locale}"

for f in po/*.po; do
    lang=$(basename "$f" .po)
    mkdir -p "$OUTDIR/$lang/LC_MESSAGES"
    msgfmt "$f" -o "$OUTDIR/$lang/LC_MESSAGES/$APP_ID.mo"
    msgfmt "$f" -o "$OUTDIR/$lang/LC_MESSAGES/$APP_ID_develop.mo"

    mkdir -p "AppDir/share/locale/$lang/LC_MESSAGES"
    msgfmt "$f" -o "AppDir/share/locale/$lang/LC_MESSAGES/$APP_ID.mo"
done
