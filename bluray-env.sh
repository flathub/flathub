#!/bin/sh

if [ -z "$LIBAACS_PATH" ]; then
  export LIBAACS_PATH=/app/share/vlc/extra/bluray/lib/libaacs
fi
if [ -z "$LIBBDPLUS_PATH" ]; then
  export LIBBDPLUS_PATH=/app/share/vlc/extra/bluray/lib/libbdplus
fi

export PATH=/app/share/vlc/extra/bluray/jre/bin:$PATH
export JAVA_HOME=/app/share/vlc/extra/bluray/jre
export LIBBLURAY_CP=/app/share/vlc/extra/bluray/share/java/
