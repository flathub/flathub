#!/bin/bash -e

SRC="/app/extra/wt.tar.gz"

# Extract
if [[ ! -f "$XDG_DATA_HOME/sha_check" ]] || [[ "$(sha256sum /app/extra/wt.tar.gz | cut -d' ' -f1)" != "$(cat "$XDG_DATA_HOME"/sha_check 2>/dev/null)" ]]; then
	sha256sum /app/extra/wt.tar.gz | cut -d' ' -f1 > "$XDG_DATA_HOME"/sha_check
	tar -xv --gzip -f "$SRC" -C "$XDG_DATA_HOME"
fi

# Execute launcher
exec "${XDG_DATA_HOME}/WarThunder/launcher"
