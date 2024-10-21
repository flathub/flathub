#!/bin/sh
export JAVA_HOME=/app/share/java_home
export LD_LIBRARY_PATH=/app/lib/art
ln -s $XDG_CACHE_HOME ~/.cache
exec android-translation-layer --gapplication-app-id=net.newpipe.NewPipe /app/share/NewPipe.apk $@
