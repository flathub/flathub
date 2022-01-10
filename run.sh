#!/bin/bash -e

SRC="/app/extra/wt.tar.gz"
FLAT_VERSION="/app/bin/version"
DATA_VERSION="${XDG_DATA_HOME}/version"

# Extract
if [[ ! -f "$DATA_VERSION" ]] || [[ `cmp --silent "$FLAT_VERSION" "$DATA_VERSION"; echo $?` -ne 0 ]] ; then
	cp -f "$FLAT_VERSION" "$DATA_VERSION"
	tar -xv --gzip -f "$SRC" -C "$XDG_DATA_HOME"
fi

# Execute launcher
exec "${XDG_DATA_HOME}/WarThunder/launcher"
