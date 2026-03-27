#!/bin/sh
export PATH=$PATH:/app/bin:/app/sbin
PY_VER=$(python3 -c "import sys; print(f\"{sys.version_info.major}.{sys.version_info.minor}\")")
export PYTHONPATH=$PYTHONPATH:/app:/app/lib/python$PY_VER/site-packages
exec python3 /app/corex/main.py "$@"
