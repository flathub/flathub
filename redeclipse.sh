#!/bin/sh

cd "/app/share/redeclipse" || exit 1
exec "./redeclipse" "$@"
