#!/bin/bash
set -e

# FlixDesk Extra-Data Processor: Extracts Widevine CDM from downloaded Chrome deb
echo "[FlixDesk Extra-Data] Extracting Widevine CDM for Netflix DRM..."

mkdir -p /app/extra
mkdir -p /tmp/chrome-extract

if [ -f "google-chrome-stable.deb" ]; then
  ar p google-chrome-stable.deb data.tar.xz | tar -xJ -C /tmp/chrome-extract ./opt/google/chrome/WidevineCdm 2>/dev/null || \
  ar p google-chrome-stable.deb data.tar.zst | tar -x --zstd -C /tmp/chrome-extract ./opt/google/chrome/WidevineCdm 2>/dev/null || \
  ar p google-chrome-stable.deb data.tar.gz | tar -xz -C /tmp/chrome-extract ./opt/google/chrome/WidevineCdm 2>/dev/null

  if [ -d "/tmp/chrome-extract/opt/google/chrome/WidevineCdm" ]; then
    cp -r /tmp/chrome-extract/opt/google/chrome/WidevineCdm /app/extra/
    echo "[FlixDesk Extra-Data] Successfully extracted Widevine CDM to /app/extra/WidevineCdm"
  else
    echo "[FlixDesk Extra-Data] Warning: Widevine directory not found in deb archive."
  fi

  rm -rf /tmp/chrome-extract
  rm -f google-chrome-stable.deb
else
  echo "[FlixDesk Extra-Data] No deb package provided in working directory."
fi
