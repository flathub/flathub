#!/bin/bash
FLATPAK_ID=${FLATPAK_ID:-"com.discordapp.DiscordPTB"}
# RPC was discontinued in DiscordPTB
#socat $SOCAT_ARGS \
#    UNIX-LISTEN:$XDG_RUNTIME_DIR/app/${FLATPAK_ID}/discord-ipc-0,forever,fork \ # check this
#    UNIX-CONNECT:$XDG_RUNTIME_DIR/discord-ipc-0 \
#    &
#socat_pid=$!

FLAGS='--enable-gpu-rasterization --enable-zero-copy --enable-gpu-compositing --enable-native-gpu-memory-buffers --enable-oop-rasterization --enable-features=UseSkiaRenderer'

if [[ $XDG_SESSION_TYPE == "wayland" ]] && [ -c /dev/nvidia0 ]
then
    FLAGS="$FLAGS --disable-gpu-sandbox"
fi

disable-breaking-updates.py
set-gtk-dark-theme.py &
env TMPDIR=$XDG_CACHE_HOME zypak-wrapper /app/discord-ptb/DiscordPTB $FLAGS "$@"
#kill -SIGTERM $socat_pid
