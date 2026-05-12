#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
MANIFEST="${SCRIPT_DIR}/al.raffi.raffi.local.yml"
STAGE_DIR="${SCRIPT_DIR}/stage"
TMP_BUILD_ROOT="/tmp/raffi-flatpak-build"
BUILD_DIR="${TMP_BUILD_ROOT}/flatpak-build"

cd "${APP_DIR}"

flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

bun run server:build
bun run build
bunx electron-builder --linux --dir

rm -rf "${STAGE_DIR}/linux-unpacked"
mkdir -p "${STAGE_DIR}/icons"
cp -a "${APP_DIR}/release/linux-unpacked" "${STAGE_DIR}/linux-unpacked"
cp -a "${APP_DIR}/build/icons/128x128.png" "${STAGE_DIR}/icons/128x128.png"
cp -a "${APP_DIR}/build/icons/512x512.png" "${STAGE_DIR}/icons/512x512.png"

rm -rf "${TMP_BUILD_ROOT}"
mkdir -p "${TMP_BUILD_ROOT}"
cp "${MANIFEST}" "${TMP_BUILD_ROOT}/al.raffi.raffi.local.yml"
cp "${SCRIPT_DIR}/run.sh" "${TMP_BUILD_ROOT}/run.sh"
cp "${SCRIPT_DIR}/al.raffi.raffi.desktop" "${TMP_BUILD_ROOT}/al.raffi.raffi.desktop"
cp "${SCRIPT_DIR}/al.raffi.raffi.metainfo.xml" "${TMP_BUILD_ROOT}/al.raffi.raffi.metainfo.xml"
cp -R "${STAGE_DIR}" "${TMP_BUILD_ROOT}/stage"

if command -v flatpak-builder >/dev/null 2>&1; then
  flatpak-builder --force-clean --user --install --install-deps-from=flathub "${BUILD_DIR}" "${TMP_BUILD_ROOT}/al.raffi.raffi.local.yml"
elif flatpak info org.flatpak.Builder >/dev/null 2>&1; then
  flatpak run --filesystem="${TMP_BUILD_ROOT}" --cwd="${TMP_BUILD_ROOT}" --command=flathub-build org.flatpak.Builder --disable-rofiles-fuse --install al.raffi.raffi.local.yml
else
  printf '%s\n' "flatpak-builder is not installed. Install it, or install org.flatpak.Builder from Flathub, then rerun this script." >&2
  exit 1
fi

printf '%s\n' "Installed al.raffi.raffi. Run it with: flatpak run al.raffi.raffi"
