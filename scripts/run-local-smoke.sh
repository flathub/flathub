#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT_DIR="${1:-"$ROOT_DIR/test-audio"}"
OUTPUT_DIR="${2:-"$ROOT_DIR/output"}"

mkdir -p "$OUTPUT_DIR"

cargo run \
  --manifest-path "$ROOT_DIR/src-tauri/Cargo.toml" \
  --example local-audio-smoke \
  -- \
  "$INPUT_DIR" \
  "$OUTPUT_DIR"
