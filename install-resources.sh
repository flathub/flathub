#!/bin/env bash

APP_ID=io.github.mujx.Nheko

install -dm755 /app/bin/
install -dm755 /app/lib/
install -dm755 /app/share/pixmaps/
install -dm755 /app/share/applications/
install -Dm755 build/nheko /app/bin/nheko
install -Dm755 resources/nheko-256.png /app/share/pixmaps/${APP_ID}.png
install -Dm755 resources/nheko.desktop /app/share/applications/${APP_ID}.desktop

install -Dm755 nheko.appdata.xml /app/share/metainfo/${APP_ID}.appdata.xml

for icon_size in 16 32 48 64 128 256 512; do
    icon_dir=/app/share/icons/hicolor/${icon_size}x${icon_size}/apps
    install -d $icon_dir
    install -m644 resources/nheko-${icon_size}.png $icon_dir/${APP_ID}.png
done

desktop-file-edit --set-icon=${APP_ID} /app/share/applications/${APP_ID}.desktop
desktop-file-validate /app/share/applications/${APP_ID}.desktop
