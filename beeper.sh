#!/bin/sh

set -oue pipefail

EXTRA_ARGS=()

# Additional args for enabling Wayland
if [[ "${BEEPER_DISABLE_WAYLAND:-0}" -eq 0 || "${XDG_SESSION_TYPE}" == "wayland" ]]; then
    EXTRA_ARGS+=(
        "--ozone-platform-hint=auto"
        "--enable-efatures=WaylandWindowDecorations"
        "--ozone-platform=wayland"
    )
fi

EXTRA_ARGS+=("--no-update")

export FLATPAK_ID="${FLATPAK_ID:-com.beeper.Beeper}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/beeper/beeper "${EXTRA_ARGS[@]}" "$@"
