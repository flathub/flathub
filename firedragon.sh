#!/bin/bash
export TMPDIR=$XDG_CACHE_HOME/tmp
exec /app/lib/firedragon/firedragon --name org.garudalinux.firedragon "$@"
