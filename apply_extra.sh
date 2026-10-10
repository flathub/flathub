#!/bin/sh
set -eu

echo "Extracting Google Antigravity binary payload..."
tar -xzf Antigravity.tar.gz --strip-components=1
rm -f Antigravity.tar.gz

echo "Google Antigravity extracted successfully."
