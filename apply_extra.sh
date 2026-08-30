#!/bin/sh
set -eu

# Install-time unpack. The Electron tree from EnoteSpace-Linux-<ver>.tar.xz
# is staged at /app/extra/main (binary: enotespace). Desktop, icon and
# metainfo are installed at build time and must not come from extra-data.

extra_root="${EXTRA_ROOT:-/app/extra}"
cd "$extra_root"

archive=enotespace.tar.xz
[ -f "$archive" ] || { echo "missing extra-data: $archive" >&2; exit 1; }

rm -rf main
mkdir main
# --no-same-owner: system-wide install runs apply_extra as root with
# capabilities dropped, so restoring archive uid/gid would fail.
tar --no-same-owner -xJf "$archive" -C main
rm -f "$archive"

[ -x main/enotespace ] || { echo "enotespace binary not found in extra-data" >&2; exit 1; }
