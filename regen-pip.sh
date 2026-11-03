#!/bin/sh
# Regenerate python3-dependencies.json from requirements.txt of the release.
set -eu

REQUIREMENTS=$(realpath "${1:?Usage: ./regen-pip.sh <path to requirements.txt>}")
cd "$(dirname "$0")"

# The generator needs requirements-parser and packaging. Install them in a temporary folder.
TMP_DIR=$(mktemp -d)
trap 'rm -rf "${TMP_DIR:?}"' EXIT
wget -q -O "$TMP_DIR/flatpak-pip-generator.py" https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/refs/heads/master/pip/flatpak-pip-generator.py
python3 -m venv "$TMP_DIR/venv"
"$TMP_DIR/venv/bin/pip" install -q requirements-parser packaging

# Keep --runtime the same as runtime-version in the manifest.
"$TMP_DIR/venv/bin/python" "$TMP_DIR/flatpak-pip-generator.py" \
        --runtime=org.freedesktop.Sdk//25.08 \
        --requirements-file="$REQUIREMENTS" \
        --ignore-pkg=psycopg2,pgspecial \
        --prefer-wheels=cryptography,bcrypt,pymssql \
        --wheel-arches=x86_64 \
        --output=python3-dependencies
