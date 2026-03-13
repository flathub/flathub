#!/bin/bash
# This script generates the generated-sources.json file.
# To use it, run: `sh ./generate-sources.sh {flow-browser-version}`
# Example: `sh ./generate-sources.sh v0.10.2`

set -euo pipefail

if [ -z "${1:-}" ]; then
  echo "Usage: $0 <version-tag>"
  echo "Example: $0 v0.10.2"
  exit 1
fi

VERSION="$1"
REPO_URL="https://github.com/MultiboxLabs/flow-browser"
TARBALL_URL="${REPO_URL}/archive/refs/tags/${VERSION}.tar.gz"
TMPDIR=$(mktemp -d)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

cleanup() {
  rm -rf "$TMPDIR"
}
trap cleanup EXIT

echo "Downloading flow-browser ${VERSION}..."
curl -sL "$TARBALL_URL" -o "$TMPDIR/source.tar.gz"

echo "Extracting bun.lock..."
tar -xzf "$TMPDIR/source.tar.gz" -C "$TMPDIR" --strip-components=1 "*/bun.lock"

echo "Generating sources..."
bunx flatpak-bun-generator@latest "$TMPDIR/bun.lock" --output "$SCRIPT_DIR/generated-sources.json"

echo "Done. generated-sources.json has been updated."
