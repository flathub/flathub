#!/bin/sh
APP=/app/ebalistyka
export LD_LIBRARY_PATH="$APP/lib:${LD_LIBRARY_PATH:-}"
exec "$APP/ebalistyka" "$@"
