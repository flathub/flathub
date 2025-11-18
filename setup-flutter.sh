#!/bin/bash
set -euo pipefail

# Optional verbose tracing in CI when DEBUG_SETUP_FLUTTER=1
if [ "${DEBUG_SETUP_FLUTTER:-0}" = "1" ]; then
  set -x
fi

usage() {
  cat <<USAGE
usage: setup-flutter.sh [-C dir] [flutter pub get args...]

This helper script sets up the Flutter SDK environment for offline builds.
It ensures flutter_tools package_config.json is properly staged and runs
flutter pub get with offline fallback.

Options:
  -C dir    Change to directory before running (default: current directory)
  -h        Show this help message
USAGE
}

OPTIND=1
TARGET_DIR="."
while getopts "C:h" opt; do
  case "$opt" in
    C)
      TARGET_DIR="$OPTARG"
      ;;
    h|*)
      usage
      exit 0
      ;;
  esac
done

shift $((OPTIND - 1))
pushd "$TARGET_DIR" >/dev/null

TOOLS_DIR="$PWD/flutter/packages/flutter_tools"
mkdir -p /var/lib/flutter/packages/flutter_tools/.dart_tool
PACKAGE_CONFIG="flutter/packages/flutter_tools/.dart_tool/package_config.json"
if [ -f "$PACKAGE_CONFIG" ]; then
  cp "$PACKAGE_CONFIG" /var/lib/flutter/packages/flutter_tools/.dart_tool/
fi

# If package_config.json wasn't present inside the SDK tree, try a staged copy
if [ ! -f /var/lib/flutter/packages/flutter_tools/.dart_tool/package_config.json ]; then
  if [ -f /run/build/lotti/package_config.json ]; then
    cp /run/build/lotti/package_config.json /var/lib/flutter/packages/flutter_tools/.dart_tool/package_config.json || true
  fi
fi

# Resolve flutter binary robustly
FLUTTER_BIN="${FLUTTER_BIN:-flutter}"
if ! command -v "$FLUTTER_BIN" >/dev/null 2>&1; then
  for cand in \
    /var/lib/flutter/bin/flutter \
    /app/flutter/bin/flutter \
    "$PWD/flutter/bin/flutter"; do
    if [ -x "$cand" ]; then
      FLUTTER_BIN="$cand"
      break
    fi
  done
fi

# Debug: show PUB_CACHE and presence of test package in cache
echo "PUB_CACHE: ${PUB_CACHE:-<unset>}"
if [ -n "${PUB_CACHE:-}" ] && [ -d "${PUB_CACHE}" ]; then
  echo "Listing PUB_CACHE contents (top-level):"
  ls -la "${PUB_CACHE}" || true
  if [ -d "${PUB_CACHE}/hosted/pub.dev" ]; then
    echo "Sample of cached hosted packages matching 'test-':"
    found=0
    for pkg in "${PUB_CACHE}/hosted/pub.dev"/test-*; do
      if [ -e "$pkg" ]; then
        basename "$pkg"
        found=1
      fi
    done
    [ $found -eq 0 ] && echo "(no cached test-* packages found)"
  fi
fi

HAVE_PKGCFG=0
if [ -f /var/lib/flutter/packages/flutter_tools/.dart_tool/package_config.json ]; then
  HAVE_PKGCFG=1
fi

if [ "$HAVE_PKGCFG" = "1" ]; then
  echo "Detected staged package_config.json for flutter_tools; skipping pub get."
else
  # Allow CI to skip offline with NO_OFFLINE_PUB=1
  if [ "${NO_OFFLINE_PUB:-0}" = "1" ]; then
    echo "NO_OFFLINE_PUB=1 set; running pub get online for flutter_tools..."
    "$FLUTTER_BIN" pub get -C "$TOOLS_DIR" "$@"
  else
    if ! "$FLUTTER_BIN" pub get --offline -C "$TOOLS_DIR" "$@"; then
      echo "Offline pub get failed, retrying with network access..." >&2
      "$FLUTTER_BIN" pub get -C "$TOOLS_DIR" "$@"
    fi
  fi
fi

popd >/dev/null