#!/bin/sh
flatpak-builder --user --ccache --force-clean --delete-build-dirs --install build com.haxxed.BasiliskII.yml
