#!/bin/sh
# Headless CLI alias — same binary with --cli (matches snap's qtmesh app).
set -eu
APP="${FLATPAK_DEST:-/app}"
exec "${APP}/bin/qtmesheditor" --cli "$@"
