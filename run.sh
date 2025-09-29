#!/bin/sh
# Ensure bundled Python + libraries are found
export PATH="/app/bin:$PATH"
export LD_LIBRARY_PATH="/app/lib:${LD_LIBRARY_PATH:-}"

exec python3 /app/bin/msxtileforge.py "$@"

