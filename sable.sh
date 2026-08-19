#!/bin/sh
export ZYPAK_CEF_LIBRARY_PATH=/app/lib/sable/libcef.so
exec zypak-wrapper /app/lib/sable/sable "$@"