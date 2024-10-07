#!/bin/bash

SCRIPT_DIR="$(dirname "$0")"

if [[ "$#" -ne 1 ]]; then
  echo "usage: $0 <sha1|sha256|...>" >&2
  exit 1
fi

$1sum | cut -d' ' -f1 | "$SCRIPT_DIR/hex-to-b64.sh"
