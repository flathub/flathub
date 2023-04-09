#!/bin/sh
# https://github.com/flathub/com.github.d4nj1.tlpui/blob/0822e6a136b18c04a6a0b5aa59fe2dc1942ccd4b/host-exec-wrapper.sh

command="$0"
host_command=${command#"/app/bin/"}
flatpak-spawn --host --clear-env "$host_command" "$@"