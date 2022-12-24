#!/bin/bash

EXTRA_ARGS=()

declare -i SIGNAL_USE_TRAY_ICON="${SIGNAL_USE_TRAY_ICON:-0}"
declare -i SIGNAL_START_IN_TRAY="${SIGNAL_START_IN_TRAY:-0}"
declare -i SIGNAL_USE_WAYLAND="${SIGNAL_USE_WAYLAND:-0}"
declare -i SIGNAL_DISABLE_GPU="${SIGNAL_DISABLE_GPU:-0}"
declare -i SIGNAL_DISABLE_GPU_SANDBOX="${SIGNAL_DISABLE_GPU_SANDBOX:-0}"

# Additional args for tray icon
if [[ "${SIGNAL_USE_TRAY_ICON}" -eq 1 ]]; then
    EXTRA_ARGS+=(
        "--use-tray-icon"
    )
fi
if [[ "${SIGNAL_START_IN_TRAY}" -eq 1 ]]; then
    EXTRA_ARGS+=(
        "--start-in-tray"
    )
fi

if [[ "${SIGNAL_USE_WAYLAND}" -eq 1 && "${XDG_SESSION_TYPE}" == "wayland" ]]; then
    EXTRA_ARGS+=(
        "--enable-features=WaylandWindowDecorations"
        "--ozone-platform=wayland"
    )
fi

if [[ "${SIGNAL_DISABLE_GPU}" -eq 1 ]]; then
    EXTRA_ARGS+=(
        "--disable-gpu"
    )
fi

if [[ "${SIGNAL_DISABLE_GPU_SANDBOX}" -eq 1 ]]; then
    EXTRA_ARGS+=(
        "--disable-gpu-sandbox"
    )
fi


echo "Debug: Will run signal with the following arguments: ${EXTRA_ARGS[@]}"
echo "Debug: Additionally, user gave: $@"

export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
exec zypak-wrapper /app/Signal_Beta/signal-desktop-beta "${EXTRA_ARGS[@]}" "$@"
