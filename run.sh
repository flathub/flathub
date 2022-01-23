#!/bin/bash -e

SRC="/app/extra/wt.tar.gz"
DIR=${CUSTOM_DIR:-$XDG_DATA_HOME}
FLAT_VERSION="/app/bin/version"
DATA_VERSION="${DIR}/WarThunder/version"

# Extract
if [[ ! -f "$DATA_VERSION" ]] || [[ `cmp --silent "$FLAT_VERSION" "$DATA_VERSION"; echo $?` -ne 0 ]] ; then
	cp -f "$FLAT_VERSION" "$DATA_VERSION"
	tar -xv --gzip -f "$SRC" -C "$DIR"
fi

# Execute launcher
exec "${DIR}/WarThunder/launcher"
