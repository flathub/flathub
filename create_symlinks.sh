#!/bin/bash
cat assets_list | while read line; do
    ln -vs ${FLATPAK_DEST}/extra/Pinball/$line ${FLATPAK_DEST}/bin/
done