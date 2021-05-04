#!/usr/bin/env bash

set -e

FLATPAK_ID=com.tracktion.Waveform

# Unpack the .deb file
mkdir -p deb-package
ar p waveform11.deb data.tar.xz | tar -xJf - -C deb-package

# Move to export/share
mkdir -p export/share
mv deb-package/usr/bin/Waveform11 waveform11
mv deb-package/usr/share/{applications,doc,mime} export/share/

# Icon
mkdir -p export/share/icons/hicolor/512x512/apps
mv deb-package/usr/share/pixmaps/* export/share/icons/hicolor/512x512/apps

rename "waveform11" "${FLATPAK_ID}" export/share/{applications,mime/packages,icons/hicolor/*/*}/waveform11.*

# Desktop file
desktop-file-edit \
    --set-key="Exec" --set-value="waveform %U" \
    --set-key="Icon" --set-value="${FLATPAK_ID}" \
    --set-key="Categories" --set-value="AudioVideo;AudioVideoEditing;" \
    --set-key="X-Flatpak-RenamedFrom" --set-value="waveform11.desktop;" \
    "export/share/applications/${FLATPAK_ID}.desktop"

# Download manager
ar p tracktion-download-manager.deb data.tar.xz | tar -xJf - -C deb-package
mv deb-package/usr/bin/tracktion-download-manager tracktion-download-manager

rm -r waveform11.deb tracktion-download-manager.deb deb-package
