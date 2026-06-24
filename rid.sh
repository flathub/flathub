#!/usr/bin/env bash

if [[ $FLATPAK_ARCH = "aarch64" ]]; then
    echo "linux-arm64"
else
    echo "linux-x64"
fi
