#!/bin/sh
# Cut-down version of steamlink.sh to avoid doing things that aren't
# going to work in a Flatpak environment

set -eu

TOP=/app/lib/steamlink

TMPDIR="$(mktemp -d)"
export TMPDIR

export PATH="$TOP/bin:$PATH"
export QTDIR="/app/lib"
export LD_LIBRARY_PATH="$TOP/lib:${LD_LIBRARY_PATH-/app/lib}"
export SDL_GAMECONTROLLERCONFIG_FILE="${XDG_DATA_HOME:-$HOME/.local/share}/Valve Corporation/SteamLink/controller_map.txt"

exit_status=0
restart=false
while true; do
    shell "$@" || exit_status="$?"

    # See if the shell wanted to launch anything
    cmdline_file="$TMPDIR/launch_cmdline.txt"
    if [ -f "$cmdline_file" ]; then
        cmd="$(cat "$cmdline_file")"
        if [ "$cmd" = "steamlink" ]; then
            restart=true
            rm -f "$cmdline_file"
            break
        else
            eval "$cmd"
            rm -f "$cmdline_file"
        fi
    else
        break
    fi
done

if [ "$restart" = "true" ]; then
    exec steamlink "$@"
fi

exit "$exit_status"
