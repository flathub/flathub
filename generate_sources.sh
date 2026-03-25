#!/bin/bash

# Check if version is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <version>"
  exit 1
fi

VERSION=$1
LOCKFILE_URL="https://raw.githubusercontent.com/jeffvli/feishin/refs/tags/v${VERSION}/pnpm-lock.yaml"
LOCKFILE="pnpm-lock.yaml"

# Download the lockfile
curl -o "$LOCKFILE" "$LOCKFILE_URL"

# Check if download was successful
if [ $? -ne 0 ]; then
  echo "Failed to download $LOCKFILE_URL"
  exit 1
fi

# Run flatpak-node-generator
flatpak-node-generator pnpm "$LOCKFILE"

# Remove the lockfile
rm "$LOCKFILE"
