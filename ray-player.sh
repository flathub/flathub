#!/bin/sh
set -e

if [ -z "${DISPLAY:-}" ]; then
    printf '%s\n' 'Ray Player requires an X11 or XWayland display.' >&2
    exit 1
fi

unset WAYLAND_DISPLAY
export GDK_BACKEND=x11
export QT_QPA_PLATFORM=xcb
export AWT_TOOLKIT=XToolkit
export JAVA_HOME=/app/jre
export PATH=/app/jre/bin:/app/bin:/usr/bin

exec /app/jre/bin/java \
    -Dfile.encoding=UTF-8 \
    -Dsun.awt.noerasebackground=true \
    -Dawt.toolkit.name=XToolkit \
    -Dcompose.layers.type=WINDOW \
    -Djpackage.app-version=1.3.296 \
    -cp '/app/lib/ray-player/*' \
    com.ray.iptv.desktop.MainKt "$@"
