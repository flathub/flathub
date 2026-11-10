#!/bin/bash
set -e

JAVA_EXEC="/app/jre/bin/java"
if [ ! -f "$JAVA_EXEC" ]; then
    JAVA_EXEC="java"
fi

export GDK_BACKEND=wayland,x11
export SKIKO_RENDER_API=OPENGL

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
