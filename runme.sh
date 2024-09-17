#!/bin/sh


export TMPDIR="$XDG_CACHE_HOME/tmp"

export PYTHONPATH="/app/opt/trelby/:/app/opt/trelby/bin:/app/opt/trelby/src"

exec "/app/opt/trelby/bin/trelby" "$@"
