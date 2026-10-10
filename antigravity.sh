#!/usr/bin/env bash
set -e

FLAGS=()

# Automatic Wayland support when running on Wayland display server
if [ "${XDG_SESSION_TYPE:-}" = "wayland" ] || [ -n "${WAYLAND_DISPLAY:-}" ]; then
    FLAGS+=("--ozone-platform-hint=auto")
fi

# Optional user-defined flags file
FLAGS_FILE="${XDG_CONFIG_HOME:-$HOME/.config}/antigravity-flags.conf"
if [ -f "$FLAGS_FILE" ]; then
    while IFS= read -r line || [ -n "$line" ]; do
        case "$line" in
            \#*|"") continue ;;
            *) FLAGS+=("$line") ;;
        esac
    done < "$FLAGS_FILE"
fi

exec zypak-wrapper.sh /app/extra/antigravity "${FLAGS[@]}" "$@"
