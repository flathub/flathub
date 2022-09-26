#!/bin/sh

PROJECT_DIR="$1"

if [ -z "${PROJECT_DIR}" ]; then
    echo "$0: Specify local rust project directory"
    exit 2
fi

./flatpak-builder-tools/cargo/flatpak-cargo-generator.py "${PROJECT_DIR}/Cargo.lock"
