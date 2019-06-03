#!/bin/sh
shopt -s nullglob

exec /app/bin/gsequencer.bin $GSEQUENCER_ARGS "$@"
