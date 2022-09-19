#!/usr/bin/env bash
set -x
# This script generates the depdendant JSON files.
# The environment variables SKYTEMPLE_DIR and SKYTEMPLE_RUST_DIR must be set to the correct paths
# of SkyTemple Randomizer and skytemple-rust in matching versions.
python3 flatpak-builder-tools/pip/flatpak-pip-generator -r $SKYTEMPLE_DIR/requirements-mac-windows.txt -o requirements-skytemple-randomizer
python3 flatpak-builder-tools/cargo/flatpak-cargo-generator.py $SKYTEMPLE_RUST_DIR/Cargo.lock -o cargo-sources.json
