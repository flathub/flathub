#!/bin/bash
for m in fz fzb fzbz fzm fzp fzpz fzz; do
    for s in 128 256; do
        install -Dm644 resources/system_icons/linux/${m}_icon${s}.png \
            /app/share/icons/hicolor/${s}x${s}/mimetypes/org.fritzing.Fritzing-application-x-fritzing-${m}.png;
    done;
    install -Dm644 resources/system_icons/${m}_icon.svg \
        /app/share/icons/hicolor/scaleable/mimetypes/org.fritzing.Fritzing-application-x-fritzing-${m}.svg;
    sed "s|\(<mime-type type=\"application/x-fritzing-${m}\">\)|\1\n<icon name=\"org.fritzing.Fritzing-application-x-fritzing-${m}\"/>|g" \
    -i /app/share/mime/packages/org.fritzing.Fritzing.xml
done
