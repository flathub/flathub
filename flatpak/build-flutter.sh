#!/usr/bin/env bash
set -euo pipefail

# Copy Flutter SDK to a writable location and merge cache artifacts
cp -a /var/lib/flutter "$FLATPAK_BUILDER_BUILDDIR/_flutter"
if [ -d "$FLATPAK_BUILDER_BUILDDIR/_flutter_sdk_cache/bin/cache" ]; then
  cp -r "$FLATPAK_BUILDER_BUILDDIR/_flutter_sdk_cache/bin/cache"/* "$FLATPAK_BUILDER_BUILDDIR/_flutter/bin/cache/"
fi

# Patch flutter_webrtc to disable -Werror (GCC 15 compatibility)
WEBRTC_PKG_DIR="$(ls -d "$PUB_CACHE/hosted/pub.dev"/flutter_webrtc-* 2>/dev/null | sort -V | tail -1)"
WEBRTC_CMAKE="${WEBRTC_PKG_DIR}/linux/CMakeLists.txt"
if [ -n "$WEBRTC_PKG_DIR" ] && [ -f "$WEBRTC_CMAKE" ]; then
  if grep -qF 'target_compile_options(${PLUGIN_NAME} PRIVATE -Wno-error)' "$WEBRTC_CMAKE"; then
    echo "$WEBRTC_CMAKE already patched"
  elif grep -qF 'apply_standard_settings(${PLUGIN_NAME})' "$WEBRTC_CMAKE"; then
    sed -i '/apply_standard_settings(\${PLUGIN_NAME})/a target_compile_options(${PLUGIN_NAME} PRIVATE -Wno-error)' "$WEBRTC_CMAKE"
  else
    echo "Failed to patch $WEBRTC_CMAKE: marker not found" >&2
    exit 1
  fi
fi