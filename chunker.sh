#!/bin/sh

APPBASE=/app/chunker
cd $APPBASE

env TMPDIR="${XDG_CACHE_HOME}" zypak-wrapper.sh /app/chunker/chunker-electron
