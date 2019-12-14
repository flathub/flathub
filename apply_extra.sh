#!/usr/bin/env sh

set -e

ar p teams.deb data.tar.xz | tar -xJf -

install -Dm0644 usr/share/pixmaps/teams.png export/share/icons/hicolor/512x512/apps/com.microsoft.Teams.png

desktop_file="export/share/applications/com.microsoft.Teams.desktop"
install -Dm0644 usr/share/applications/teams.desktop "$desktop_file"
desktop-file-edit --set-key="Exec" --set-value="teams %U" "$desktop_file"
desktop-file-edit --set-key="Icon" --set-value="com.microsoft.Teams" "$desktop_file"

mv usr/bin .
mkdir share
mv usr/share/teams share/teams

rm -rf teams.deb usr
