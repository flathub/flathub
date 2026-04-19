#!/bin/bash

export JAVA_HOME=/app/jre
export PATH=/app/jre/bin:/app/bin:/app/netlogo/bin:/usr/bin

# export JAVA_TOOL_OPTIONS="
# -Dsun.java2d.uiScale=1.5
# -Dawt.useSystemAAFontSettings=on
# -Dswing.aatext=true
# -Dsun.java2d.dpiaware=true
# "

# export GDK_BACKEND=x11

cd /app/netlogo

if [ -f "/app/netlogo/bin/NetLogo" ]; then
    exec /app/netlogo/bin/NetLogo "$@"
else
    exec /app/netlogo/netlogo-gui.sh "$@"
fi
