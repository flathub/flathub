#!/bin/bash
# ZERAVRIX Flatpak Launcher
# Optimized for sandbox security, Wayland/X11 compatibility, and hardware acceleration
set -e

# Java runtime bundled via Flatpak SDK extension
JAVA_EXEC="/usr/lib/sdk/openjdk21/bin/java"
if [ ! -f "$JAVA_EXEC" ]; then
    JAVA_EXEC="java"
fi

# Detect Wayland or X11 and set appropriate Skiko options
export GDK_BACKEND=wayland,x11
export SKIKO_RENDER_API=OPENGL

# Execute with strictly bounded memory footprint and G1GC
exec "$JAVA_EXEC" \
    -Xms128m \
    -Xmx768m \
    -XX:+UseG1GC \
    -XX:+UseStringDeduplication \
    -XX:MaxGCPauseMillis=20 \
    -Dskiko.renderApi=OPENGL \
    -Dskiko.linux.autodetect=true \
    -Dawt.useSystemAAFontSettings=on \
    -Dswing.aatext=true \
    -Dfile.encoding=UTF-8 \
    -cp "/app/lib/zeravrix/*" \
    com.zeravrix.zeravrix.MainKt "$@"
