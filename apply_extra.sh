#!/usr/bin/env bash

set -e

FLATPAK_ID=com.tracktion.Waveform13

# Unpack the .deb file
mkdir -p deb-package
ar p waveform13.deb data.tar.gz | tar -zxf - -C deb-package

# Move executable to root dir
mv deb-package/usr/bin/Waveform13 Waveform13

# Export necessary share items
mkdir -p export/share
mv deb-package/usr/share/{doc,mime} export/share/
rename "waveform13" "${FLATPAK_ID}" export/share/mime/packages/waveform13.xml


# Download manager
ar p tracktion-download-manager.deb data.tar.xz | tar -xJf - -C deb-package
mv deb-package/usr/bin/tracktion-download-manager tracktion-download-manager

rm -r waveform13.deb tracktion-download-manager.deb deb-package
