#!/bin/sh

echo "If we need to do anything custom..."
file /app/bin/overlayed-wrapped
ldd /app/bin/overlayed-wrapped

ls -hal /app/bin
ls -hal /app

exec /app/bin/overlayed-wrapped "$@"