#!/bin/sh
exec flatpak-spawn --directory="$(pwd)" --host \
    flatpak run  org.videolan.VLC "$@"
