#!/bin/bash
export PYTHONPATH="/app/lib/python3.12/site-packages:/app/share/rootChat:$PYTHONPATH"
exec python3 /app/share/rootChat/app.py "$@"
