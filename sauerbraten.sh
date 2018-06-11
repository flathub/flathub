#!/bin/sh

cd "/app/share/sauerbraten" || exit 1
exec "./sauerbraten" -q"$HOME/.sauerbraten" "$@"
