#!/usr/bin/env bash

set -e

FLATPAK_ID=com.tracktion.Waveform13

function extract_deb () {
    cat $1 | bsdtar x $2
    tar -xf $2 -C deb-package
    rm $2 # Clean up tar archive
}

# Unpack the .deb file
mkdir -p deb-package
extract_deb waveform13.deb data.tar.gz

# Move executable to root dir
mv deb-package/usr/bin/Waveform13 Waveform13

# Export necessary share items
mkdir -p export/share
mv deb-package/usr/share/{doc,mime} export/share/
rename "waveform13" "${FLATPAK_ID}" export/share/mime/packages/waveform13.xml


# Download manager
extract_deb tracktion-download-manager.deb data.tar.xz
mv deb-package/usr/bin/tracktion-download-manager tracktion-download-manager

rm -r waveform13.deb tracktion-download-manager.deb deb-package