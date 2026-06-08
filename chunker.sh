#!/bin/sh

APPBASE=/app/chunker
cd $APPBASE

export ELECTRON_OZONE_PLATFORM_HINT="auto"

env TMPDIR="${XDG_CACHE_HOME}" zypak-wrapper.sh /app/chunker/chunker-electron
