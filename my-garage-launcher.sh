#!/bin/sh
set -eu

export LD_LIBRARY_PATH="/app/lib/my_garage/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export GTK_USE_PORTAL=1
exec /app/lib/my_garage/my_garage "$@"
