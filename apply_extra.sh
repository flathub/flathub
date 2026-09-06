#!/bin/sh
set -eu
for f in tapsign-x64.tar.xz tapsign-arm64.tar.xz tapsign.tar.xz; do
  if [ -f "$f" ]; then
    tar -xJf "$f"
    rm -f "$f"
  fi
done
