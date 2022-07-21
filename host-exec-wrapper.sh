#!/bin/sh

command="$0"
host_command=${command#"/app/bin/"}
flatpak-spawn --host --clear-env "$host_command" "$@"
