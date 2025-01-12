#!/usr/bin/env bash

set -euo pipefail

TARGET_VERSION="${1:-0.101.0-beta6}"

LOCK_FILE_DIR=$(mktemp -d)
cleanup() {
  rm -r "$LOCK_FILE_DIR"
}
trap cleanup EXIT
set -x
curl -s -L "https://github.com/doukutsu-rs/doukutsu-rs/archive/refs/tags/$TARGET_VERSION.tar.gz" | tar xzf - -C "$LOCK_FILE_DIR"
podman run --rm -it \
  -v .:/tmp/build:Z \
  -v "$LOCK_FILE_DIR:$LOCK_FILE_DIR:Z" \
  docker.io/library/python:3.12.8 \
  sh -c "pip install aiohttp toml && /tmp/build/flatpak-builder-tools/cargo/flatpak-cargo-generator.py ${LOCK_FILE_DIR}/doukutsu-rs-${TARGET_VERSION}/Cargo.lock -o /tmp/build/cargo-sources.json"
