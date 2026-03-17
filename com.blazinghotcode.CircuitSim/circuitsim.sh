#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${DISPLAY:-}" ]]; then
    for x_socket in /tmp/.X11-unix/X*; do
        if [[ -S "$x_socket" ]]; then
            DISPLAY=":${x_socket##*X}"
            export DISPLAY
            break
        fi
    done
fi

if [[ -z "${DISPLAY:-}" ]]; then
    printf 'CircuitSim requires X11 or XWayland inside Flatpak, but no DISPLAY was available.\n' >&2
    exit 1
fi

exec /app/jre/bin/java -Dfile.encoding=UTF-8 -Djava.awt.headless=false -jar /app/lib/CircuitSim.jar "$@"
