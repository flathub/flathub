#!/bin/bash
# SPDX-License-Identifier: MIT

# cause script to fail as soon as one command has a failing exit code,
# rather than trying to continue. See: https://stackoverflow.com/a/1379904/
set -e

if [ "${FLATPAK_ARCH}" == "x86_64" ]; then
  arch="x64"
elif [ "${FLATPAK_ARCH}" == "aarch64" ]; then
  arch="arm64"
else
  echo "Unsupported Arch present $FLATPAK_ARCH"
  exit 1
fi

npm run package -- --platform="linux" --arch="$arch"
