#!/usr/bin/env bash

if ! command -v base64 >/dev/null 2>&1; then
    echo "\`base64\` needs to be on the \$PATH for this script to work."
    exit 1
fi

if ! command -v curl >/dev/null 2>&1; then
    echo "\`curl\` needs to be on the \$PATH for this script to work."
    exit 1
fi

if ! command -v hexdump >/dev/null 2>&1; then
    echo "\`hexdump\` needs to be on the \$PATH for this script to work."
    exit 1
fi

if ! command -v yq >/dev/null 2>&1; then
    echo "\`yq\` needs to be on the \$PATH for this script to work."
    exit 1
fi

set -euo pipefail

BASE_URL="https://download.todesktop.com/2003241lzgn20jd"
INDEX="${BASE_URL}/latest-linux.yml"
LATEST_SHA512=""
LATEST_URL=""

SOURCES_PATH=".modules.[1].sources[0]"
SOURCES_URL="${SOURCES_PATH}.url"
SOURCES_SHA512="${SOURCES_PATH}.sha512"

MANIFEST_PATH="./com.beeper.Beeper.yml"

LATEST_URL="${BASE_URL}/$(curl -s "${INDEX}" | yq -r '.path')"
LATEST_SHA512="$(curl -s "${INDEX}" | yq -r '.sha512' | base64 -d | hexdump -v -e '/1 "%02x"')"

if [ -z "${LATEST_URL}" ] || [ -z "${LATEST_SHA512}" ]; then
    echo "Failed to retrieve the latest version information."
    exit 1
fi

if [ ! -f "${MANIFEST_PATH}" ]; then
    echo "Manifest file not found."
    exit 1
fi

yq -y -i "${SOURCES_URL} = \"${LATEST_URL}\"" "${MANIFEST_PATH}"
yq -y -i "${SOURCES_SHA512} = \"${LATEST_SHA512}\"" "${MANIFEST_PATH}"
