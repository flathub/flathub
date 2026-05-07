#!/bin/bash
# AICHE Desktop Flatpak launcher.
#
# Resolves the application root in two layouts:
#   /app/extra/aiche-desktop  → Flathub extra-data install (production)
#   /app/lib/aiche-desktop    → local build (path: ..)

if [ -d "/app/extra/aiche-desktop" ]; then
    APPLIB="/app/extra/aiche-desktop"
else
    APPLIB="/app/lib/aiche-desktop"
fi

PYVER=$(python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")

export PYTHONPATH="${APPLIB}:/app/lib/python${PYVER}/site-packages:/usr/lib/python3/dist-packages:/usr/lib/python${PYVER}/site-packages"
export PYTHONNOUSERSITE=1

if [ "$XDG_SESSION_TYPE" = "wayland" ] || [ -n "$WAYLAND_DISPLAY" ]; then
    export QT_QPA_PLATFORM=wayland
else
    export QT_QPA_PLATFORM=xcb
fi

export YDOTOOL_SOCKET="${XDG_RUNTIME_DIR}/aiche-ydotool.socket"
if [ ! -S "$YDOTOOL_SOCKET" ]; then
    /app/bin/ydotoold --socket-path="$YDOTOOL_SOCKET" &
    for i in 1 2 3 4 5; do
        [ -S "$YDOTOOL_SOCKET" ] && break
        sleep 0.2
    done
fi

export PATH="/app/lib/ffmpeg/bin:${PATH}"
export AICHE_FLATPAK=1
export AICHE_PRODUCTION_MODE="${AICHE_PRODUCTION_MODE:-true}"
export QT_LOGGING_RULES="*=false"

cd "${APPLIB}"
exec python3 -m src.main "$@"
