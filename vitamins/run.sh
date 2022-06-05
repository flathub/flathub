#!/bin/bash
FLATPAK_ID=${FLATPAK_ID:-"io.spacingbat3.webcord"}
socat $SOCAT_ARGS \
    UNIX-LISTEN:$XDG_RUNTIME_DIR/app/${FLATPAK_ID}/discord-ipc-0,forever,fork \
    UNIX-CONNECT:$XDG_RUNTIME_DIR/discord-ipc-0 \
    &
socat_pid=$!

FLAGS='--enable-gpu-rasterization --enable-zero-copy --enable-gpu-compositing --enable-native-gpu-memory-buffers --enable-oop-rasterization --enable-features=UseSkiaRenderer'

if [[ $XDG_SESSION_TYPE == "Wayland" ]] && [ -c /dev/nvidia0 ]
then
    FLAGS="$FLAGS --disable-gpu-sandbox"
fi

env TMPDIR=$XDG_CACHE_HOME zypak-wrapper /app/bin/webcord $FLAGS "$@"
kill -SIGTERM $socat_pid