#!/bin/sh
export ZYPAK_CEF_LIBRARY_PATH=/app/extra/sable/libcef.so
exec zypak-wrapper /app/extra/sable/sable "$@"
