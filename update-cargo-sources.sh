#!/usr/bin/env bash
for cmd in git curl cargo; do
    hash $cmd 2>/dev/null || { echo >&2 "error: $cmd not found"; exit 1; }
done

set -e

VERSION=$(sed -n 's/^version=//p' plugin.info)
TMPDIR=$(mktemp -d)
trap '{ rm -rf -- "$TMPDIR"; }' EXIT

git clone --depth 1 --branch v$VERSION https://github.com/gonicus/gouda-matrix $TMPDIR
curl -o $TMPDIR/flatpak-cargo-generator.py https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/refs/heads/master/cargo/flatpak-cargo-generator.py

python3 $TMPDIR/flatpak-cargo-generator.py "$TMPDIR/Cargo.lock" --yaml -o "cargo-sources.yml"

git add cargo-sources.yml
