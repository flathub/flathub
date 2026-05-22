#!/bin/bash
if [ -z "$DISPLAY" ]; then
    for sock in /tmp/.X11-unix/X*; do
        [ -S "$sock" ] && export DISPLAY=":${sock##*X}" && break
    done
fi
export LD_LIBRARY_PATH=/app/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
cd /app/share/astroshelf
exec python3 /app/share/astroshelf/main.py "$@"
