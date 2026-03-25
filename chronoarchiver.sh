#!/bin/sh
export PYTHONPATH=/app/share/chronoarchiver/src
exec python3 /app/share/chronoarchiver/src/ui/app.py "$@"
