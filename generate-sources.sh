#!/usr/bin/env bash

git clone https://github.com/acristoffers/void-rs
git clone https://github.com/flatpak/flatpak-builder-tools

pushd void-rs || exit
git checkout "$(yq -r '.modules[0].sources[1].commit' ../me.acristoffers.void.yml)"
popd || exit

pushd flatpak-builder-tools/cargo || exit

uv sync
uv run flatpak-cargo-generator.py -o ../../cargo-sources.json ../../void-rs/Cargo.lock

popd || exit

rm -rf void-rs flatpak-builder-tools
