#!/bin/sh
export PATH="/app/bin:$PATH"
exec python3 /app/lib/downnux-manager/scripts/downnux-desktop.py "$@"

