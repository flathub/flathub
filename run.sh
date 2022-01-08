#!/bin/bash -e

DIR="$XDG_DATA_HOME/Game/"

mkdir -p $DIR
ls /app/extra/
tar -xv --gzip -f /app/extra/wt.tar.gz -C $DIR

exec "${DIR}launcher"

