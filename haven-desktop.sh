#!/bin/sh
# Haven: mirrors im.riot.Riot's own element.sh wrapper (the upstream Flathub manifest this repo
# is forked from) - zypak-wrapper is what lets Electron's own Chromium sandbox work inside the
# Flatpak sandbox (org.electronjs.Electron2.BaseApp provides it on $PATH).

FLAGS=""

if [ "$XDG_SESSION_TYPE" = "wayland" ]; then
    FLAGS="$FLAGS --enable-wayland-ime --ozone-platform-hint=auto --enable-features=WaylandWindowDecorations,WebRTCPipeWireCapturer"
    if [ -e /dev/nvidia0 ]; then
        FLAGS="$FLAGS --disable-gpu-sandbox"
    fi
fi

if [ -n "$https_proxy" ]; then
    FLAGS="$FLAGS --proxy-server=$https_proxy"
elif [ -n "$http_proxy" ]; then
    FLAGS="$FLAGS --proxy-server=$http_proxy"
fi

env TMPDIR="$XDG_RUNTIME_DIR/app/${FLATPAK_ID:-software.haven.HavenDesktop}" \
    zypak-wrapper /app/Haven/haven-desktop $FLAGS "$@"
