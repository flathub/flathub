#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODELS_DIR="$ROOT_DIR/src-tauri/models"
MODEL_FILENAME="htdemucs.onnx"
MODEL_PATH="$MODELS_DIR/$MODEL_FILENAME"
MODEL_URL="https://github.com/thedavidweng/openkara-models/releases/download/model-v1.0.0/htdemucs.onnx"
MODEL_SHA256="66eebc928e4a9f7e5b837605794f4db2b91748f163b0bcbc8f29a526ffe5607e"

require_tool() {
  local tool="$1"

  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "error: required tool '$tool' is not installed" >&2
    exit 1
  fi
}

verify_checksum() {
  local file_path="$1"
  local actual_checksum

  actual_checksum="$(shasum -a 256 "$file_path" | awk '{print $1}')"
  [[ "$actual_checksum" == "$MODEL_SHA256" ]]
}

require_tool curl
require_tool node
require_tool shasum

mkdir -p "$MODELS_DIR"

if [[ -f "$MODEL_PATH" ]]; then
  if verify_checksum "$MODEL_PATH"; then
    echo "Model already present and verified at $MODEL_PATH"
    node "$ROOT_DIR/scripts/prepare-onnx-runtime.mjs"
    exit 0
  fi

  echo "error: existing model at $MODEL_PATH failed SHA-256 verification" >&2
  echo "error: remove the file and rerun scripts/setup.sh to fetch a clean copy" >&2
  exit 1
fi

tmp_file="$(mktemp "$MODELS_DIR/$MODEL_FILENAME.download.XXXXXX")"

cleanup() {
  rm -f "$tmp_file"
}

trap cleanup EXIT

echo "Downloading $MODEL_FILENAME to $tmp_file"
curl -L --fail --progress-bar "$MODEL_URL" -o "$tmp_file"

if ! verify_checksum "$tmp_file"; then
  echo "error: downloaded model checksum mismatch" >&2
  echo "error: expected $MODEL_SHA256" >&2
  exit 1
fi

mv "$tmp_file" "$MODEL_PATH"
trap - EXIT

echo "Model verified and saved to $MODEL_PATH"
node "$ROOT_DIR/scripts/prepare-onnx-runtime.mjs"
